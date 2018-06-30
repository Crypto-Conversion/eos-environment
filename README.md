## Development Environment for EOS

### I. Create and Compile

```
git clone git@github.com:7flash/eos-environment.git
npm install
npm link
./create.sh project-name
```

#### AssemblyScript

```
https://gist.github.com/toonsevrin/26498311ee024afbca0d890badf5f4f8
```

#### C++
```
docker run -v ~./contracts:/opt/eosio/bin/data-dir -it eosio/eos-dev
cd /opt/eosio/bin/data-dir
eosiocpp project-name && cd project-name
eosiocpp -o project-name.wast project-name.cpp
eosiocpp -g project-name.abi  project-name.cpp
```

### II. Deploy
```
cd project-name
vim ./config.js
node ./deploy.js
```

### III. Test
```
node ./test.js
```