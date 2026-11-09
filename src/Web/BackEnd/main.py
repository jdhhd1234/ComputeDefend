import os
import secrets
import subprocess
from pathlib import Path

import redis
from fastapi import Depends, FastAPI, Header, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles

from pydantic import BaseModel

class ProcessInfo(BaseModel):
    pid: int
    name: str

app = FastAPI()

REDIS_URL = os.getenv("REDIS_URL") or os.getenv("KV_URL")
API_KEY = os.getenv("API_KEY")
PROCESSES_KEY = "processes"

# 설정이 빠진 채 공개되면 인증 없이 열리므로 Vercel에서는 시작을 막는다.
if os.getenv("VERCEL") and not (REDIS_URL and API_KEY):
    raise RuntimeError("Vercel 환경변수 REDIS_URL과 API_KEY를 설정해야 합니다.")

# 서버리스 함수는 요청 간 메모리를 공유하지 않으므로 Vercel에서는 Redis에 저장한다.
redis_client = redis.from_url(REDIS_URL, decode_responses=True) if REDIS_URL else None

# 로컬 개발용 저장소. PID를 키로 저장해 C++ 클라이언트를 다시 실행해도 중복되지 않게 한다.
processes: dict[int, dict] = {}

def require_api_key(x_api_key: str | None = Header(default=None)) -> None:
    if not API_KEY:
        return

    if x_api_key is None or not secrets.compare_digest(x_api_key.encode(), API_KEY.encode()):
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

@app.post("/api/processes", dependencies=[Depends(require_api_key)])
def main(processinfo: ProcessInfo) -> dict:

    data = {
        "name": processinfo.name,
        "pid": processinfo.pid
    }

    if redis_client:
        redis_client.hset(PROCESSES_KEY, str(processinfo.pid), processinfo.name)
    else:
        processes[processinfo.pid] = data

    return data

@app.get("/api/processes", dependencies=[Depends(require_api_key)])
def get_processes() -> list[dict]:
    if redis_client:
        stored = redis_client.hgetall(PROCESSES_KEY)
        return [{"name": stored[pid], "pid": int(pid)} for pid in sorted(stored, key=int)]

    return list(processes.values())

# API 라우트보다 뒤에 둬야 /api 경로를 가리지 않는다.
app.mount(
    "/",
    StaticFiles(directory=Path(__file__).resolve().parents[1] / "FrontEnd", html=True),
    name="frontend",
)