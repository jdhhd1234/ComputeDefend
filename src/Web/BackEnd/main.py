import os
import secrets
import subprocess

from pathlib import Path
from fastapi import Depends, FastAPI, Header, HTTPException
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
# fastapi dev src/Web/BackEnd/main.py

@app.post("/api/processes", dependencies=[Depends(require_api_key)])
def main(processinfos: list[ProcessInfo]) -> list[dict]:
    global processes

    processes = {
        info.pid: {"name": info.name, "pid": info.pid}
        for info in processinfos
    }

    return list(processes.values())

@app.get("/api/processes", dependencies=[Depends(require_api_key)])
def get_processes() -> list[dict]:
    return list(processes.values())

# API 라우트보다 뒤에 둬야 /api 경로를 가리지 않는다.
app.mount(
    "/",
    StaticFiles(directory=Path(__file__).resolve().parents[1] / "FrontEnd", html=True),
    name="frontend",
)