import axios from 'axios';

const customerId = 3;

const price = 1;
const times = 1000;

const url = 'http://127.0.0.1';
const ports = [3000, 3001];

const responseBefore = await axios.get(`${url}:${ports[0]}/clientes/${customerId}/extrato`);
const totalBefore = responseBefore.data.saldo.total;

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

const total = response.data.saldo.total;

console.log(`Got ${total}`);
const expectedDifference = -price * times;
const expected = totalBefore + expectedDifference;
console.log(`Expected: ${expected}`);
if (total !== expected) {
    console.error(`Test failed, got ${total}\n`);
    console.error("(make sure the test is the only thing calling the server while it's running)");
} else {
    console.log('Test passed!');
}
