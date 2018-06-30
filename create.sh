mkdir $1
touch $1/$1.js
touch $1/config.json
touch $1/deploy.js
touch $1/test.js

cat <<EOF >$1/$1.js
const eos = require('../eos')

const config = require('./config.json')

let contract = null

module.exports = async () => {
  if(contract === null) {
    contract = await eos.contract(config.account)
  }

  return contract
}
EOF

cat <<EOF >$1/config.json
{
	"account": "$1",
	"directory": "../contracts/$1",
	"name": "$1",
	"accountCreated": true,
	"creator": "sevenflash",
	"owner": "EOS6iLS23J5WXwDX2ybgZmDkPvTN8XuYxh6fcYLjnFwx5BD6kXsk7",
	"active": "EOS6J2765xNSyjNi26QHPrj851FKYxuy88jE37ZWaCeLuQmtv9Lwn",
	"stake": 100,
	"bytes": 819200
}
EOF

cat <<EOF >$1/deploy.js
const fs = require('fs')
const path = require('path')
const deploy = require('../deploy')

let config = null

new Promise((resolve, reject) => {
  fs.readFile(path.resolve('./config.json'), (err, res) => {
    if(err)
      return reject(err)

    resolve(res)
  })
}).then((result) => {
  config = JSON.parse(result)

  return deploy(config)
}).then(() => {
  if(config.accountCreated === false) {
    config.accountCreated = true

    return new Promise(
      resolve => fs.writeFile(
        path.resolve('./config.json'),
        JSON.stringify(config, null, "\t"),
        {encoding: 'utf8', flag: 'w'},
        resolve
      )
    )
  }
}).then(() => {
  console.log('$1 done')
}).catch(e => {
  console.error(e)
})
EOF

cat <<EOF >$1/test.js
const assert = require('assert')

const eos = require('../eos')
const $1 = require('./$1')
const config = require('./config.json')

const options = {
  authorization: `${config.creator}@active`,
  broadcast: true,
  sign: true
}

describe('$1', function() {
  this.timeout(0)

  before(async () => {
    this.contract = await $1()
  })

  it('case', async () => {
    const result = await this.contract.hi(config.creator, options)

    assert.equal(result.transaction_id.length > 0, true)
  })
})

EOF