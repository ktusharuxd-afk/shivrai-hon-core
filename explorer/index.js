const express = require('express');
const axios = require('axios');
const cors = require('cors');
const app = express();
app.use(cors());
app.use(express.static('public'));
const RPC_URL = process.env.RPC_URL || 'http://shivraiuser:honpassword123@127.0.0.1:18555';
async function rpc(method, params = []) {
    const res = await axios.post(RPC_URL, {jsonrpc: '1.0', id: 'explorer', method, params});
    return res.data.result;
}
app.get('/api/info', async (req, res) => {
    try { const info = await rpc('getblockchaininfo'); res.json(info); }
    catch(e) { res.status(500).json({ error: e.message }); }
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
    } catch(e) { res.status(500).json({ error: e.message }); }
});
app.listen(3000, () => console.log('SHIVRAI HON Explorer running on port 3000'));
