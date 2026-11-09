
const num_list = document.querySelector('#num_list');
const process_list = document.querySelector('#process_list');
const test_button = document.querySelector('#test_button')

process_list.remove();

async function loadProcesses() {
    try {
        const response = await fetch('http://127.0.0.1:8000/api/processes');

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
    }
}

loadProcesses();
test_button.addEventListener('click', loadProcesses);