import express from 'express';
const app = express();
app.use(express.json());

const user = {
    id: 1,
    limit: 100000,
    total: 0,
    oldestTransaction: 0,
    transactions: [] as Transaction[],
};

type User = typeof user;

interface Transaction {
    valor: number;
    tipo: string;
    descricao: string;
    realizada_em: string;
};

for (let i = 0; i < 10; i++) {
    user.transactions.push({} as Transaction);
}

function createTransaction(valor: number, tipo: string, descricao: string) {
    const transaction = { valor, tipo, descricao, realizada_em: new Date().toISOString() };
    user.transactions[user.oldestTransaction] = transaction;
    user.oldestTransaction = (user.oldestTransaction + 1) % user.transactions.length;
}

app.post('/clientes/:id/transacoes', (req, res) => {
    const { valor, tipo, descricao } = req.body;
    createTransaction(valor, tipo, descricao);
    if (tipo === 'd') {
        if (user.total - valor < -user.limit) {
            return res.status(422).send({ message: 'Saldo insuficiente' });
        }
        user.total -= valor;
    } else {
        user.total += valor;
    }
    res.send({ limite: user.limit, saldo: user.total });
});

app.get('/clientes/:id/extrato', (req, res) => {
    res.send({
        saldo: {
            total: user.total,
            data_extrato: new Date().toISOString(),
            limite: user.limit,
        },
        ultimas_transacoes: user.transactions.filter(t => t.valor !== undefined),
    });
});

app.listen(9999, () => {
    console.log('Server is running on port 9999');
});
