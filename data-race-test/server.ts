import express from 'express';
const app = express();
app.post('/clientes/:id/transacoes', (req, res) => {
    res.send('Hello World!');
});

app.get('/clientes/:id/extrato', (req, res) => {
    res.send({ saldo: 0 });
});

app.listen(9999, () => {
    console.log('Server is running on port 9999');
});
