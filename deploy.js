const fs = require('fs')
const path = require('path')

const eos = require('./eos')

const source = async (directory, name) => {
  const wastPath = path.join(process.cwd(), directory, name.concat('.wasm'))
  const abiPath = path.join(process.cwd(), directory, name.concat('.abi'))

  const wast = new Promise(resolve => {
    fs.readFile(wastPath, (_, r) => resolve(r))
  })
  const abi = new Promise(resolve => {
    fs.readFile(abiPath, (_, r) => resolve(r))
  })

  return Promise.all([wast, abi])
}

const deploy = async (config) => {
  const { directory, name, account, accountCreated, creator, owner, active, stake, bytes } = config

  const [ wast, abi ] = await source(directory, name)

  if(!wast || !abi)
    throw new Error("Source files not found")

  await eos.transaction((tx) => {
    if (!accountCreated) {
      tx.newaccount({
        creator,
        owner,
        active,
        name: account
      })
      
      tx.buyrambytes({
        payer: creator,
        receiver: account,
        bytes: bytes
      })
      
      tx.delegatebw({
        from: creator,
        receiver: account,
        stake_net_quantity: `${stake}.0000 EOS`,
        stake_cpu_quantity: `${stake}.0000 EOS`,
        transfer: 0
      })
    }

    tx.setcode({
      account: account,
      code: wast,
      vmtype: 0,
      vmversion: 0
    })

    tx.setabi({
      account: account,
      abi: JSON.parse(abi)
    })
  }).catch(e => {
    console.error(e)
  })
}

module.exports = deploy