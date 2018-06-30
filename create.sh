mkdir $1
touch $1/$1.js
touch $1/config.js
touch $1/deploy.js
touch $1/test.js

cat <<EOF >$1/$1.js
const config = require('./config')

let contract = null

module.exports = async () => {
  if(contract === null) {
    contract = await eos.contract(config.account)
  }

  return contract
}
EOF

cat <<EOF >$1/config.js
module.exports = {
  "account": "$1",
  "directory": "../contracts/$1",
  "name": '$1'
}
EOF

cat <<EOF >$1/deploy.js
const eos = require('./eos')

const config = require('./config')

const deploy = require('../deploy')(config)

deploy().then(() => {
  console.log('$1 done')
}).catch(e => {
  console.error(e)
})
EOF

cat <<EOF >$1/test.js
const eos = require('./eos')
const $1 = require('./$1')

describe('$1', () => {
  before(async () => {
    this.contract = await $1()
  })
})
EOF