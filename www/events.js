let canvas;
let ctx;
// 绘制红点
function drawRedDot(x, y) {
    ctx.beginPath();
    ctx.arc(x, y, 5, 0, 2 * Math.PI);
    ctx.fillStyle = 'red';
    ctx.fill();
}
document.addEventListener('DOMContentLoaded', function () {
    canvas = document.getElementById('desktopCanvas');
    canvas.width = 1920;
    canvas.height = 1080;
    ctx = canvas.getContext('2d');
    // 禁止鼠标滚轮缩放
    window.addEventListener('wheel', function (event) {
        if (event.ctrlKey) {
            event.preventDefault();
        }
    }, { passive: false });
    document.addEventListener('contextmenu', function (event) {
        event.preventDefault(); // 阻止右键菜单的默认行为
    });
    // bind keyboard event
    window.addEventListener('keydown', async (event) =>  {
        await keyboard(Math.trunc(Mstsc.scancode(event)), true);
        event.preventDefault();
    });
    window.addEventListener('keyup', async (event) =>  {
        await keyboard(Math.trunc(Mstsc.scancode(event)), false);
        event.preventDefault();
    });
    canvas.addEventListener('mousemove', async (event) => {
        const rect = canvas.getBoundingClientRect();
        const scaleX = canvas.width / rect.width;
        const scaleY = canvas.height / rect.height;
        const x = (event.clientX - rect.left) * scaleX;
        const y = (event.clientY - rect.top) * scaleY;

        drawRedDot(x, y);
        await mouseMove(
            Math.trunc(x),
            Math.trunc(y));
    });
    canvas.addEventListener('mousedown', async (event) => {
        await mouseDown(
            Math.trunc(event.button),
            Math.trunc(event.offsetX),
            Math.trunc(event.offsetY));
    });

    canvas.addEventListener('mouseup', async (event) => {
        await mouseUp(
            Math.trunc(event.button),
            Math.trunc(event.offsetX),
            Math.trunc(event.offsetY));
    });


    const fileUploadButton = document.getElementById('fileUploadButton');
    const portMapButton = document.getElementById('portMapButton');
    const cmdButton = document.getElementById('cmdButton');


    fileUploadButton.addEventListener('click', async () => {
        const destinationPath = document.getElementById('destinationPath').value;
        const sourceFilePath = document.getElementById('sourceFilePath').value;
        await uploadFile(destinationPath, sourceFilePath);
    });
    portMapButton.addEventListener('click', async () => {
        const portMap = document.getElementById('portMap').value;
        await setPortMap(portMap);
    });
    cmdButton.addEventListener('click', async () => {
        const cmdline = document.getElementById('cmdline').value;
        await command(cmdline);
    });

});

window.drawImageOnCanvas = async (imageUrl) => {
    const img = new Image();
    img.src = imageUrl;

    img.onload = function () {
        ctx.drawImage(img, 0, 0);
    };
};
// 用于添加通知的函数
window.addNotification = async(message) =>{
    const notificationBar = document.getElementById('notification-bar');
    const notification = document.createElement('div');
    notification.classList.add('notification');
    const now = new Date();
    const hours = String(now.getHours()).padStart(2, '0');
    const minutes = String(now.getMinutes()).padStart(2, '0');
    const seconds = String(now.getSeconds()).padStart(2, '0');
    const timeString = `${hours}:${minutes}:${seconds}`;
    notification.innerHTML = `<${timeString}> ${message}`;
    notificationBar.appendChild(notification);
    notificationBar.scrollTop = notificationBar.scrollHeight;
}
