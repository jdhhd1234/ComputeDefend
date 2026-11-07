
const num_list = document.querySelector('#num_list');
const process_list = document.querySelector('#process_list');
const test_button = document.querySelector('#test_button')

// 임시 목록 데이터 (나중에 백엔드 응답으로 교체)
const items = ['항목 1', '항목 2', '항목 3'];

process_list.remove();
for (const text of items) {
    const li = document.createElement('li');
    li.textContent = text;
    num_list.appendChild(li);
}

test_button.addEventListener('click', async function() {
    try {
        const response = await fetch('http://127.0.0.1:8000');

        if (!response.ok) 
        {
            throw new Error(`HTTP Error! State Code: ${response.status}`);
        }

        const data = await response.json();
        console.log(data);
    } catch (error) {
        console.error('통신 에러:', error);
    }
});