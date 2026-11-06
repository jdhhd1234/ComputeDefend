
const process_list = document.querySelector('#process_list');
const test_button = document.querySelector('#test_button')

const newProcess = document.createElement('ul');

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