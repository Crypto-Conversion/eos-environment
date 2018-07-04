#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/crypto.h>

namespace eosio {
    class swaponline : public contract {
        private:
            struct swap
            {
                account_name swapID;
                account_name eosOwner;
                account_name btcOwner;
                uint64_t requiredDeposit;
                uint64_t currentDeposit;
                checksum160 secretHash;
                checksum160 secret;
                uint8_t status = 0; // 0 = open, 1 = active, 2 = withdrawn, 3 = refunded

                account_name primary_key() const { return swapID; }
            };

            multi_index<N(swap), swap> _swaps;
        public:
            swaponline(account_name self)
            : contract(self),
            _swaps(self, self)
            {}

            const uint32_t TIMEOUT = 5*60;

            void open(account_name eosOwner, account_name btcOwner, account_name quantity, checksum160& secretHash);
            void withdraw(account_name swapID, checksum160& secret);
            void refund(account_name swapID);
            void deposit(const currency::transfer& t, account_name code);

            void apply(account_name contract, account_name action);
    };

    void swaponline::open(
        account_name eosOwner,
        account_name btcOwner,
        account_name quantity,
        checksum160& secretHash
    )
    {
        require_auth(eosOwner);

        for (auto& item : _swaps) {
            if(item.eosOwner == eosOwner && item.btcOwner == btcOwner) {
                eosio_assert(item.status == 3 || item.status == 4,
                "Between participants already exists open swap");
            }
        }

        _swaps.emplace(eosOwner, [&](auto& item) {
                item.swapID = _swaps.available_primary_key();
                item.eosOwner = eosOwner;
                item.btcOwner = btcOwner;
                item.requiredDeposit = quantity;
                item.currentDeposit = 0;
                item.secretHash = secretHash;
                item.status = 0;
        });
    }

    void swaponline::withdraw(account_name swapID, checksum160& secret)
    {
        auto swapIterator = _swaps.find(swapID);
        eosio_assert(swapIterator != _swaps.end(), "Swap not found");

        require_auth(swapIterator->btcOwner);

        eosio_assert(swapIterator->status != 0, "EOS Owner should deposit funds");
        eosio_assert(swapIterator->status != 2, "Funds was already withdrawn");
        eosio_assert(swapIterator->status != 3, "Funds was refunded");

        assert_ripemd160((char*)&secret, sizeof(secret), (const checksum160*)&swapIterator->secretHash);

        account_name recipient = swapIterator->btcOwner;
        uint64_t quantity = swapIterator->currentDeposit;

        _swaps.modify(swapIterator, swapIterator->eosOwner, [&](auto& item) {
            item.status = 2;
            item.secret = secret;
            item.currentDeposit = 0;
        });

        action(
            permission_level{_self, N(active)},
            N(eosio.token), N(transfer),
            std::make_tuple(_self, recipient, quantity, std::string(""))
        ).send();
    }

    void swaponline::refund(account_name swapID)
    {
        auto swapIterator = _swaps.find(swapID);

        eosio_assert(swapIterator != _swaps.end(), "Swap not found");
        eosio_assert(swapIterator->status != 0, "Refund is allowed only after full deposit");
        eosio_assert(swapIterator->status != 2, "Funds was withdrawn");
        eosio_assert(swapIterator->status != 3, "Funds was already refunded");

        require_auth(swapIterator->eosOwner);

        account_name recipient = swapIterator->eosOwner;
        uint64_t quantity = swapIterator->currentDeposit;

        _swaps.modify(swapIterator, swapIterator->eosOwner, [&](auto& item) {
            item.status = 3;
            item.currentDeposit = 0;
        });

        action(
            permission_level{_self, N(active)},
            N(eosio.token), N(transfer),
            std::make_tuple(_self, recipient, quantity, std::string(""))
        ).send();
    }

    void swaponline::deposit(const currency::transfer& transfer, account_name contract)
    {
        eosio_assert(transfer.to == contract, "Funds should be transfered to contract");

        auto swapIterator = _swaps.find(stoll(transfer.memo));

        eosio_assert(swapIterator != _swaps.end(), "Swap not found");
        eosio_assert(swapIterator->status == 1, "Swap has been funded already");

        _swaps.modify(swapIterator, swapIterator->eosOwner, [&](auto& item) {
            item.currentDeposit += transfer.quantity.amount;

            if(item.currentDeposit >= item.requiredDeposit) {
                item.status = 1;
            }
        });
    }

    void swaponline::apply(account_name contract, account_name action)
    {
        if(action == N(transfer)) {
            deposit(unpack_action_data<currency::transfer>(), contract);
            return;
        }

        auto& thiscontract = *this;
        switch(action) {
            EOSIO_API(swaponline, (open)(withdraw)(refund))
        };
    }
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        eosio::swaponline swap(receiver);
        swap.apply(code, action);
        eosio_exit(0);
    }
}