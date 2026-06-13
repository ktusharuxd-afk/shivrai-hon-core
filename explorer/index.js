const express = require('express');
const axios = require('axios');
const cors = require('cors');
const app = express();
app.use(cors());
app.use(express.static('public'));
const RPC_URL = process.env.RPC_URL || 'http://shivraiuser:honpassword123@127.0.0.1:18555';
async function rpc(method, params = []) {
    const res = await axios.post(RPC_URL, {jsonrpc: '1.0', id: 'explorer', method, params}, {timeout: 5000});
    return res.data.result;
}
app.get('/api/info', async (req, res) => {
    try {
        const info = await rpc('getblockchaininfo');
        res.json({...info, online: true});
    } catch(e) {
        res.json({blocks: '—', chain: 'regtest', difficulty: 0, online: false, error: 'Node offline'});
    }
});
app.get('/api/latest', async (req, res) => {
    try {
        const info = await rpc('getblockchaininfo');
        const blocks = [];
        let hash = info.bestblockhash;
        for(let i = 0; i < 10 && hash; i++) {
            const block = await rpc('getblock', [hash]);
            blocks.push(block);
            hash = block.previousblockhash;
        }
        res.json(blocks);
    } catch(e) { res.json([]); }
});

app.use(express.json());
app.post('/api/rpc', async (req, res) => {
    try {
        const {method, params, rpcUrl, rpcUser, rpcPass} = req.body;
        const url = rpcUrl || 'http://127.0.0.1:8332';
        const auth = Buffer.from((rpcUser||'shivraiuser')+':'+(rpcPass||'honpassword123')).toString('base64');
        const result = await axios.post(url, {jsonrpc:'1.0',id:'miner',method,params}, {
            headers:{Authorization:'Basic '+auth},
            timeout:30000
        });
        res.json({result: result.data.result, error: null});
    } catch(e) {
        res.json({result: null, error: e.message});
    }
});
app.listen(3000, () => console.log('SHIVRAI HON Explorer running on port 3000'));
