const eos = require('../eos')

const config = require('./config.json')

let contract = null

module.exports = async () => {
  if(contract === null) {
    contract = await eos.contract(config.account)
  }

  return contract
}
