const fs = require('fs')
const path = require('path')

const eos = require('./eos')

const newaccount = ({ creator, owner, active, account }) => ({
  account: 'eosio.system',
  name: 'newaccount',
  data: {
    creator,
    owner,
    active,
    name: account
  }
})

const buyrambytes = ({ creator, account }) => ({
  account: 'eosio.system',
  name: 'buyrambytes',
  data: {
    payer: creator,
    receiver: account,
    bytes: 8192
  }
})

const delegatebw = ({ creator, account, stake }) => ({
  account: 'eosio.system',
  name: 'delegatebw',
  data: {
    from: creator,
    receiver: account,
    stake_net_quantity: `${stake}.0000 EOS`,
    stake_cpu_quantity: `${stake}.0000 EOS`,
    transfer: 0
  }
})

const setcode = ({ account, wast }) => ({
  account: 'eosio.system',
  name: 'setcode',
  data: {
    account: account,
    code: wast,
    vmtype: '0',
    vmversion: '0'
  }
})

const setabi = ({ account, abi }) => ({
  account: 'eosio',
  name: 'setabi',
  data: {
    account: account,
    abi: JSON.parse(abi)
  }
})

const source = async (directory, name) => {
  const wastPath = path.join(process.cwd(), directory, name.concat('.wast'))
  const abiPath = path.join(process.cwd(), directory, name.concat('.abi'))

  const wast = new Promise(resolve => {
    fs.readFile(wastPath, (_, r) => resolve(r.toString()))
  })
  const abi = new Promise(resolve => {
    fs.readFile(abiPath, (_, r) => resolve(r))
  })

  return Promise.all([wast, abi])
}

const deploy = async (config) => {
  const { directory, name, account, accountCreated, creator, owner, active, stake } = config

  const [ wast, abi ] = await source(directory, name)

  if(!wast || !abi)
    throw new Error("Source files not found")

  let actions = []
  if(!accountCreated) {
    actions.push(newaccount({ creator, owner, active, account }))
    actions.push(buyrambytes({ creator, account }))
    actions.push(delegatebw({ creator, account, stake }))
  }
  actions.push(setcode({ account, wast }))
  actions.push(setabi({ account, abi }))

  console.log(actions)

  await eos.transaction((tx) => ({ actions }))
}

module.exports = deploy