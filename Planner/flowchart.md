# 기본적인 아키텍쳐 흐름.


flowchart LR
    부모웹제어[시작] -> 프로세스확인{조건확인}
    프로세스확인 -> JavaScript Fetch
    JavaScript Fetch -> Python Server
    Python Server -> SubProcess
    SubProcess -> C/C++ EXE Controler[끝]