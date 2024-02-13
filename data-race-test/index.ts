import axios from 'axios';

const customerId = 3;

const price = 1;
const times = 1000;

const url = 'http://127.0.0.1';
const ports = [3000, 3001];

const promises = [];
console.time('run test');
for (let i = 0; i < times; i++) {
    console.log(`Request ${i}`);
    const endpoint = `clientes/${customerId}/transacoes`;
    const port = ports[i % 2];
    const req = axios.post(
        `${url}:${port}/${endpoint}`,
        { valor: price, tipo: 'd', descricao: `teste${i}` }
    );
    promises.push(req);
}

console.log('Waiting for requests to finish...');
await Promise.all(promises);

console.log('All requests finished!');
const response = await axios.get(`${url}:${ports[0]}/clientes/${customerId}/extrato`);
console.timeEnd('run test');

console.log(`Got ${response.data.saldo.total}`);
const expected = -price * times;
console.log(`Expected: ${expected}`);
if (response.data.saldo.total !== expected) {
    console.error(`Test failed, got ${response.data.saldo.total}`);
} else {
    console.log('Test passed!');
}
