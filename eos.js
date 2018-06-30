const fs = require('fs')
const Eos = require('eosjs')
const binaryen = require('binaryen')

const config = require('./config.js')

const eos = Eos({
  binaryen,

  keyProvider: config.keyProvider,
  httpEndpoint: config.httpEndpoint,
  chainId: config.chainId
})

module.exports = eos