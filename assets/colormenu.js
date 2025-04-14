const fs = require('fs');

function menu() {
    const str = fs.readFileSync('menu.chars', {encoding: 'utf16le'})

    const lines = str.split('\n');

    lines[0] = lines[0].split('').slice(1).join('');

    const data = lines.map(l => l.split('').map(c => c == '$' ? 4 : (['/', '|', '_', '\\', '<', '>'].includes(c) ? 14 : 15))).flat();

    while (data.length != 4800) data.push(15);

    const data_arr = new Uint8Array(data);

    fs.writeFileSync('menu.colors', data_arr, {encoding: null});

}

function game() {
    const str = fs.readFileSync('game.chars', {encoding: 'utf16le'})

    const lines = str.split('\n');

    lines[0] = lines[0].split('').slice(1).join('');

    const data = lines.map(l => l.split('').map(c => c == '█' ? 14 : 15)).flat();

    while (data.length != 4800) data.push(15);

    const data_arr = new Uint8Array(data);

    fs.writeFileSync('game.colors', data_arr, {encoding: null});

}

game();

