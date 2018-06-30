import "allocator/arena"

import { env as EOS, ISerializable, Contract } from "./eoslib/eoslib"
import { DataStream } from "./eoslib/datastream"
import { printstr, N, assert } from "./eoslib/utils"
import { Create, Withdraw, Refund } from "./eoslib/actions"

export function apply(receiver: u64, code: u64, action: u64): void {
    var swap: Swap = new Swap(receiver)
    swap.apply(code, action)
}

export class Swap extends Contract {
    on_transfer(args: Transfer): void {
        EOS.db_update_i64(args.user, args.participant, "balances", args.amount)
    }

    // call this method only after on_transfer
    on_create(args: Create): void {
        EOS.require_auth(args.user)

        let it = EOS.db_find_i64(args.user, args.participant, "balances")

        assert(it >= 0)

        EOS.db_update_i64(args.user, args.participant, "secrets", args.secretHash)
    }

    on_refund(args: Refund): void {
        EOS.require_auth(args.user)

        let balance = EOS.db_find_i64(args.user, args.participant, "balances")

        // transfer token to user when participant didn't reveal his secret
        eosio.token.transfer(self, args.user)
    }

    on_withdraw(args: Withdraw): void {
        EOS.require_auth(args.user)

        let balance = EOS.db_find_i64(args.user, args.participant, "balances")
        let participant = EOS.db_find_i64(args.user, args.participant, "participant")

        let secretHash = EOS.db_find_i64(args.user, args.participant, "secrets")

        assert(sha256(args.secret) == secretHash)

        EOS.db_update_i64(args.user, args.participant, "secretsRevealed", args.secret)

        eosio.token.transfer(self, participant)

        EOS.db_remove_i64(balance)
    }

    get_secret(): void {
        return EOS.db_find_i64(args.user, args.participant, "secretsRevealed")
    }

    apply(code: u64, action: u64): void {
        if (code == self && action == N("create")) {
            this.on_create()
        } else if(code == self && action == N("withdraw")) {
            this.on_withdraw()
        } else if (code == self && action == N("refund")) {
            this.on_refund()
        } else if (code == N(eosio.token) && action == N("transfer")) {
            this.on_transfer()
        }
    }

}

