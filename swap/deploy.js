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
  console.log('swap done')
}).catch(e => {
  console.error(e)
})
