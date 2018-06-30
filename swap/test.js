const assert = require('assert')

const eos = require('../eos')
const swap = require('./swap')
const config = require('./config.json')

const options = {
  authorization: config.creator + '@active',
  broadcast: true,
  sign: true
}

describe('swap', function() {
  this.timeout(0)

  before(async () => {
    this.contract = await swap()
  })

  it('case', async () => {
    const result = await this.contract.hi(config.creator, options)

    assert.equal(result.transaction_id.length > 0, true)
  })
})

