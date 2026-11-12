import asyncio
import json
import os
import secrets
import subprocess

from pathlib import Path
from fastapi import Depends, FastAPI, Header, HTTPException
from fastapi.responses import StreamingResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles

from pydantic import BaseModel

class ProcessInfo(BaseModel):
    pid: int
    name: str

app = FastAPI()

API_KEY = os.getenv("API_KEY")

# 설정이 빠진 채 공개되면 인증 없이 열리므로 Vercel에서는 시작을 막는다.
if os.getenv("VERCEL") and not API_KEY:
    raise RuntimeError("Vercel 환경변수 API_KEY를 설정해야 합니다.")

# 메모리 저장소. C++ 클라이언트가 보낸 최신 앱 목록으로 통째로 교체해 종료된 앱이 남지 않게 한다.
# Vercel에서는 인스턴스마다 따로 존재하고 인스턴스가 종료되면 사라진다.
processes: dict[int, dict] = {}

# 실시간 목록을 받는 브라우저 SSE 연결마다 큐 하나
clients: set[asyncio.Queue] = set()

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

@app.post("/api/processes", dependencies=[Depends(require_api_key)])
async def main(processinfos: list[ProcessInfo]) -> list[dict]:
    global processes

    new_processes = {
        info.pid: {"name": info.name, "pid": info.pid}
        for info in processinfos
    }

    # 목록이 바뀌었을 때만 연결된 브라우저에 푸시한다.
    if new_processes != processes:
        processes = new_processes
        data = list(processes.values())
        for queue in clients:
            queue.put_nowait(data)

    return list(processes.values())

@app.get("/api/processes", dependencies=[Depends(require_api_key)])
def get_processes() -> list[dict]:
    return list(processes.values())

# 브라우저 EventSource는 헤더를 못 붙이므로 API 키를 쿼리로 받는다.
@app.get("/api/processes/stream")
async def stream_processes(api_key: str | None = None):
    if not is_valid_key(api_key):
        raise HTTPException(status_code=401, detail="Invalid API key")

    queue: asyncio.Queue = asyncio.Queue()
    queue.put_nowait(list(processes.values()))  # 연결 직후 현재 목록 한 번
    clients.add(queue)

    async def events():
        try:
            while True:
                data = await queue.get()
                yield f"data: {json.dumps(data)}\n\n"
        finally:
            # 브라우저가 연결을 끊으면 여기로 와서 정리된다.
            clients.discard(queue)

    return StreamingResponse(events(), media_type="text/event-stream")

# API 라우트보다 뒤에 둬야 /api 경로를 가리지 않는다.
app.mount(
    "/",
    StaticFiles(directory=Path(__file__).resolve().parents[1] / "FrontEnd", html=True),
    name="frontend",
)