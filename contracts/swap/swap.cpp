#include <eosiolib/eosio.hpp>
#include <eosiolib/types.h>
#include <eosiolib/crypto.h>
#include <eosiolib/currency.hpp>
#include <eosiolib/transaction.hpp>

class swaponline : public eosio::contract {
    public:
        swaponline( account_name self )
        :contract(self),_trades( _self, _self){}

        //@abi action
        void openTrade(
            account_name eosParticipant,
            account_name btcParticipant,

            uint64_t amount,
            checksum160 secretHash)
        {
            require_auth(eosParticipant);

            for(auto& trade : _trades)
            {
                if(trade.eosParticipant == eosParticipant &&
                    trade.btcParticipant == btcParticipant &&
                    trade.status == 0)
                {
                    eosio_assert(false, "Between participants already exists open trade");
                    return;
                }

                _trades.emplace(eosParticipant, [&](auto& trade) {
                    trade.key = _trades.available_primary_key();
                    trade.eosParticipant = eosParticipant;
                    trade.btcParticipant = btcParticipant;
                    trade.requiredDeposit = amount;
                    trade.currentDeposit = 0;
                    trade.secretHash = secretHash;
                    trade.secret = "";
                    trade.status = 0;
                });
            }
        }

        //@abi action
        void withdraw(uint64_t tradeID, const char* secret)
        {
            auto it = _trades.find(tradeID);
            eosio_assert(it != _trades.end(), "Trade does not exists")

            require_auth(it->btcParticipant);

            eosio_assert(it->status == 1, "Participant should reserve required amount of tokens for trade");

            assert_ripemd160( (char*)&secret, sizeof(secret), (const checksum160*)&it->secretHash);

            account_name recipient = it->btcParticipant;
            uint64_t amount = it->currentDeposit;

            eosio_assert(amount > 0, "Nothing to refund");

            _trades.modify(it, it->eosParticipant, [&](auto& trade) {
                trade.status = 2;
                trade.secret = secret;
                trade.currentDeposit = 0;
            });

            action(
                permission_level{_self, N(active)},
                N(eosio.token), N(transfer),
                std::make_tuple(_self, recipient, amount, std::string(""))
            ).send();
        }

        //@abi action
        void refund(uint64_t tradeID)
        {
            auto it = _trades.find(tradeID);

            eosio_assert(it != _trades.end(), "Trade does not exists")
            eosio_assert(it->status == 0 || it->status == 1, "Trade was executed");

            account_name recipient = it->eosParticipant;
            uint64_t amount = it->currentDeposit;

            eosio_assert(amount > 0, "Nothing to refund");

            require_auth(it->eosParticipant);

            _trades.modify(it, it->eosParticipant, [&](auto& trade) {
                trade.status = 3;
                trade.currentDeposit = 0;
            });

            action(
                permission_level{_self, N(active)},
                N(eosio.token), N(transfer),
                std::make_tuple(_self, recipient, amount, std::string(""))
            ).send();
        }

        //@abi action
        template<typename T>
        void onDeposit(uint64_t receiver, const T& transfer)
        {
            eosio_assert(transfer.to == receiver, "Deposit should be sent to contract");

            auto it = _trades.find(transfer.memo);

            eosio_assert(it != _trades.end(), "Trade does not exists");
            eosio_assert(it->status == 0, "Trade was funded already");

            _trades.modify(it, it->eosParticipant, [&](auto& trade) {
                trade.currentDeposit += transfer.quantity;

                if(trade.currentDeposit >= trade.requiredDeposit) {
                    trade.status = 1;
                }
            });
        }

    private:
        //@abi table trade i64
        struct trade
        {
            uint64_t tradeID;
            account_name eosParticipant;
            account_name btcParticipant;
            uint64_t requiredDeposit;
            uint64_t currentDeposit;
            checksum160 secretHash;
            uint8_t status = 0; // 0 = was opened, 1 = has full deposit, 2 = funds was withdrawn, 3 = funds was refunded
            char secret[];

            uint64_t primary_key() const { return tradeID; }
        };
        typedef eosio::multi_index<N(trade), trade> tradesTable;

        trade findTrade(uint64_t tradeID)
        {
            auto it = _trades.find(tradeID);
            eosio_assert(it != _trades.end(), "EOS Participant should open a trade");

            return *it;
        }

        tradesTable _trades;
};

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        swap
        if(code == N(eosio.token) && action == N(transfer)) {
            onDeposit(receiver, unpack_action_data<eosio::token::transfer_args>());
            return;
        }

        switch(action) [
            EOSIO_API(swaponline, (openTrade)(withdraw)(refund))
        };
    }
}