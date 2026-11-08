import subprocess

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from pydantic import BaseModel

class ProcessInfo(BaseModel):
    pid: int
    name: str

app = FastAPI()

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

@app.get("/")
def main():
    return {"Hello": "World"}

@app.post("/api/processes")
async def main(processinfo: ProcessInfo) -> dict:

    data = {
        "name": processinfo.name,
        "pid": processinfo.pid
    }

    return data