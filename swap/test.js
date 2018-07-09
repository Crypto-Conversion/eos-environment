const ripemd160 = require('bitcoinjs-lib/src/crypto').ripemd160;
const assert = require('assert')
const eos = require('../eos')
const swap = require('./swap')
const config = require('./config.json')

const options = {
  authorization: config.creator + '@active',
  broadcast: true,
  sign: true
}

const eosOwner = 'sevenflash'
const btcOwner = 'sevenflash55'

const lastID = 0;
const swapID = lastID + 1;

const amount = 100;

const secret = hash('c0809ce9f484fdcdfb2d5aabd609768ce0374ee97a1a5618ce4cd3f16c00a078')
const secretHash = hash(secret)

const contract = config.account;

function quantity(amount) {
  const decimals = 4;
  return amount * 10 ** decimals;
}

function asset(amount) {
  return `${amount}.0000 EOS`;
}

function hash(secret) {
  return ripemd160(Buffer.from(secret, 'hex')).toString('hex')
}

describe('swap', function() {
  this.timeout(0)

  before(async () => {
    this.contract = await swap()
  });

  it('open atomic swap between btcOwner and eosOwner', async () => {
    await this.contract.open(eosOwner, btcOwner, quantity(amount), secretHash, options);
  });

  it('check that swap was saved in the table', async () => {
    const result = await eos.getTableRows({
      code: contract,
      scope: contract,
      table: 'swap',
      json: true
    });

    const swap = result.rows.filter((row) => row.swapID == swapID).pop();

    assert.equal(swap.btcOwner, btcOwner);
    assert.equal(swap.eosOwner, eosOwner);
    assert.equal(swap.swapID, swapID);
    assert.equal(swap.secret, 0000000000000000000000000000000000000000);
    assert.equal(swap.requiredDeposit, quantity(amount));
    assert.equal(swap.currentDeposit, 0);
    assert.equal(swap.status, 0);
  });

  it('transfer funds to the contract with memo', async () => {
    const result = await eos.transfer(eosOwner, 'swaponline11', asset(amount), swapID);

    assert.ok(result.transaction);
  });

  it('check that deposit was processed', async () => {
    const result = await eos.getTableRows({
      code: contract,
      scope: contract,
      table: 'swap',
      json: true
    });

    const swap = result.rows.filter((row) => row.swapID == swapID).pop();

    assert.equal(swap.status, 1);
  });

  it('withdraw funds revealing the secret', async () => {
    await this.contract.withdraw(swapID, secret, options);
  });

  it('refund funds instead of withdrawal', async () => {
    await this.contract.refund(swapID, options);
  })
})