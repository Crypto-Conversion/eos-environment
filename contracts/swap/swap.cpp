#include <math.h>
#include <eosiolib/dispatcher.hpp>

// https://tbfleming.github.io/cib/eos.html#gist=d230f3ab2998e8858d3e51af7e4d9aeb
// https://github.com/eoscanada/shine/blob/master/contract/shine.cpp#L96-L97

namespace swaponline {
    void exchange::openSwap(account_name user, account_name participant, checksum256& secretHash, extended_asset quantity) {
       eosio_assert(quantity.is_valid());
       eosio_assert(_accounts.checkBalance(user, quantity));
       eosio_assert(_accounts.hasNoDeals(user));

       _accounts.openDeal(user, participant);
    }

    void exchange::withdrawFunds(account_name user, checksum256& secret) {
        require_auth(user);
        // reveal secret
        // withdraw funds
        action(
            permission_level{_self, N(active)},
            N(eosio.token), N(transfer),
            std::make_tuple(_self, to, quantity, std::string(""))
        ).send();
        _accounts.setSecret(user, secret);
        _accounts.closeDeal(user, participant);
    }

    void exchange::refundFunds() {
        // refund funds to user
    }

    void onTransfer(const currency::transfer& t, account_name code) {
        if(t.to == _this_contract) {
            auto a = extended_asset(t.quantity, code);
            _accounts.adjust_balance(t.from, a, t.memo);
        }
    }

    void swap::apply(account_name contract, account_name act) {
        if(act == N(transfer)) {
            onTransfer(unpack_action_data<currency::transfer>(), contract);
            return;
        }

        if(contract != _this_contract)
            return;

        switch(act) {
            EOSIO_API(swap, (open)(withdraw)(refund))
        };

        switch(act) {
            case N(open):
                openSwap();
                return;

            case N(withdraw):
                withdrawFunds();
                return;

            case N(refund):
                refundFunds();
                return;
        };
    }
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        swaponline::swap swap(receiver);
        swap.apply(code, action);
        eosio_exit(0);
    }
}