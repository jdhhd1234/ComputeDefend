import asyncio
import json
import os
import secrets

from pathlib import Path
from fastapi import Depends, FastAPI, Header, HTTPException
from fastapi.responses import StreamingResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles

from pydantic import BaseModel

class ProcessInfo(BaseModel):
    pid: int
    name: str

class KillCommand(BaseModel):
    name: str
    pid: int

app = FastAPI()

API_KEY = os.getenv("API_KEY")

# 설정이 빠진 채 공개되면 인증 없이 열리므로 Vercel에서는 시작을 막는다.
if os.getenv("VERCEL") and not API_KEY:
    raise RuntimeError("Vercel 환경변수 API_KEY를 설정해야 합니다.")


# ---------------------------------------------------------------------------
# 저장소
# Vercel은 인스턴스가 여러 개라 메모리를 공유하지 못한다. 그래서 Redis(Upstash)에
# 프로세스 목록과 종료 명령 큐를 둔다. Redis 환경변수가 없으면(로컬) 메모리를 쓴다.
# ---------------------------------------------------------------------------

PROCESSES_KEY = "computer_defender:processes"
COMMANDS_KEY = "computer_defender:commands"

# Vercel Marketplace의 Upstash는 KV_REST_API_* 이름으로, Upstash 콘솔은 UPSTASH_REDIS_REST_* 이름으로 준다.
REDIS_URL = os.getenv("UPSTASH_REDIS_REST_URL") or os.getenv("KV_REST_API_URL")
REDIS_TOKEN = os.getenv("UPSTASH_REDIS_REST_TOKEN") or os.getenv("KV_REST_API_TOKEN")

# 여러 인스턴스에서 목록이 서로 달라지지 않게 Vercel에서는 Redis를 필수로 한다.
if os.getenv("VERCEL") and not (REDIS_URL and REDIS_TOKEN):
    raise RuntimeError("Vercel에서 Upstash Redis 환경변수(KV_REST_API_URL, KV_REST_API_TOKEN)를 설정해야 합니다.")


class RedisStore:
    def __init__(self, url: str, token: str):
        from upstash_redis.asyncio import Redis

        self.redis = Redis(url=url, token=token, allow_telemetry=False)

    async def get_processes(self) -> list[dict]:
        raw = await self.redis.get(PROCESSES_KEY)
        return json.loads(raw) if raw else []

    async def set_processes(self, data: list[dict]) -> None:
        await self.redis.set(PROCESSES_KEY, json.dumps(data))

    async def push_command(self, command: dict) -> None:
        await self.redis.rpush(COMMANDS_KEY, json.dumps(command))

    async def take_commands(self) -> list[dict]:
        # LPOP count는 원자적이라 C++이 두 번 가져가도 같은 명령이 중복 실행되지 않는다.
        raw = await self.redis.lpop(COMMANDS_KEY, 100)
        if not raw:
            return []
        if isinstance(raw, str):
            raw = [raw]
        return [json.loads(item) for item in raw]


class MemoryStore:
    def __init__(self):
        self.processes: list[dict] = []
        self.commands: list[dict] = []

    async def get_processes(self) -> list[dict]:
        return self.processes

    async def set_processes(self, data: list[dict]) -> None:
        self.processes = data

    async def push_command(self, command: dict) -> None:
        self.commands.append(command)

    async def take_commands(self) -> list[dict]:
        commands, self.commands = self.commands, []
        return commands


store = RedisStore(REDIS_URL, REDIS_TOKEN) if REDIS_URL and REDIS_TOKEN else MemoryStore()


def is_valid_key(key: str | None) -> bool:
    if not API_KEY:
        return True
    return key is not None and secrets.compare_digest(key.encode(), API_KEY.encode())

def require_api_key(x_api_key: str | None = Header(default=None)) -> None:
    if not is_valid_key(x_api_key):
        raise HTTPException(status_code=401, detail="Invalid API key")

origins = [
    "http://127.0.0.1:5500",
    "http://127.0.0.1:8000"
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# uv run fastapi dev
# .venv\Scripts\python -m uvicorn src.Web.BackEnd.main:app --reload
# uvicorn src.Web.BackEnd.main:app --reload

# C++ 클라이언트가 보낸 최신 앱 목록으로 통째로 교체해 종료된 앱이 남지 않게 한다.
@app.post("/api/processes", dependencies=[Depends(require_api_key)])
async def main(processinfos: list[ProcessInfo]) -> list[dict]:
    data = [{"name": info.name, "pid": info.pid} for info in processinfos]
    await store.set_processes(data)
    return data

@app.get("/api/processes", dependencies=[Depends(require_api_key)])
async def get_processes() -> list[dict]:
    return await store.get_processes()

# 브라우저 → 서버: 종료 명령을 큐에 넣는다. 목록에 없는 프로세스는 거절한다.
@app.post("/api/processes/kill", dependencies=[Depends(require_api_key)])
async def kill_process(command: KillCommand) -> dict:
    current = await store.get_processes()
    if not any(p["pid"] == command.pid and p["name"] == command.name for p in current):
        raise HTTPException(status_code=404, detail="Process not found")

    await store.push_command({"name": command.name, "pid": command.pid})
    return {"queued": True}

# 서버 → C++: 쌓인 종료 명령을 한 번에 넘기고 비운다.
@app.get("/api/commands", dependencies=[Depends(require_api_key)])
async def take_commands() -> list[dict]:
    return await store.take_commands()

# 브라우저 EventSource는 헤더를 못 붙이므로 API 키를 쿼리로 받는다.
# 인스턴스끼리 직접 알릴 방법이 없으므로 1초마다 저장소를 확인해 바뀌었을 때만 보낸다.
@app.get("/api/processes/stream")
async def stream_processes(api_key: str | None = None):
    if not is_valid_key(api_key):
        raise HTTPException(status_code=401, detail="Invalid API key")

    async def events():
        last = None
        while True:
            data = await store.get_processes()
            if data != last:
                last = data
                yield f"data: {json.dumps(data)}\n\n"
            await asyncio.sleep(1)

    return StreamingResponse(events(), media_type="text/event-stream")

# API 라우트보다 뒤에 둬야 /api 경로를 가리지 않는다.
app.mount(
    "/",
    StaticFiles(directory=Path(__file__).resolve().parents[1] / "FrontEnd", html=True),
    name="frontend",
)
