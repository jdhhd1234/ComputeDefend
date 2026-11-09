
const num_list = document.querySelector('#num_list');
const process_list = document.querySelector('#process_list');
const test_button = document.querySelector('#test_button')
const api_key_input = document.querySelector('#api_key');

// Live Server(5500)에서는 로컬 FastAPI로, 배포 환경에서는 같은 도메인으로 요청한다.
const is_live_server = location.port === '5500';
const api_base = is_live_server ? 'http://127.0.0.1:8000' : '';

process_list.remove();

async function loadProcesses() {
    try {
        const response = await fetch(`${api_base}/api/processes`, {
            headers: { 'X-API-Key': api_key_input.value }
        });

        if (!response.ok)
        {
            throw new Error(`HTTP Error! State Code: ${response.status}`);
        }

        const processes = await response.json();

        num_list.replaceChildren();
        for (const { name, pid } of processes) {
            const li = document.createElement('li');
            li.textContent = `${name} (PID: ${pid})`;
            num_list.appendChild(li);
        }
    } catch (error) {
        console.error('통신 에러:', error);

        const li = document.createElement('li');
        li.textContent = `불러오기 실패: ${error.message}`;
        num_list.replaceChildren(li);
    }
}

// 배포 환경은 API 키 입력이 필요하므로 버튼을 눌렀을 때만 불러온다.
if (is_live_server) {
    loadProcesses();
}
test_button.addEventListener('click', loadProcesses);