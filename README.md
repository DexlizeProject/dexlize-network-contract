# dexlize-network-contract


### Usage (Resting the order of buy/sell and cancel the order of buy/sell)


### Resting order bought Example
you can add order bought by transfering the token of EOS, meanwhile the token need to be bought that 
would be set in the memo. 

```js
contract: eosio.token
action: transfer
data: {
    "from": "yourself",
    "to": "dexlizenetwk",
    "quantity": "100.0000 EOS",
    "memo": {"type": "0", "amount": "10000.0000", "symbol": "ELE", "contract": "elementscoin"}
}
```

### Resting order selled Example
you can add the order selled by transfering the token that want to be selled, meanwhile the eos token
you want to be cost that would be set in the memo.

```js
contract: elementscoin
action: transfer
data: {
    "from": "yourself",
    "to": "dexlizenetwk"
    "quantity": "10000.0000 ELE",
    "memo": {"type": "1", "amount": "10000.0000", "symbol": "EOS", "contract": "eosio.token"}
}
```

### Cancel order selled Example
you can cancel the order selled by setting the type of bill in the memo.

```js
contract: dexlizenetwk
action: cancel
data: {
    "from": "yourself",
    "bill_id": "100001",
    "memo": "sell"
}
```

### Cancel order bought Example
you can cancel the order bought by setting the type of bill in the memo.

```js
contract: dexlizenetwk
action: cancel
data: {
    "from": "yourself",
    "bill_id": "100001",
    "memo": "buy"
}
```

### Buy token Example
You can buy token that you want to buy meanwhile you set the type and the id of order selled.

```js
contract: eosio.token
action: transfer
data: {
    "from": "yourself",
    "to": "dexlizenetwk"
    "quantity": "10000.0000 EOS",
    "memo": {"type": "3", "id": "10001"}
}
```

### Sell token Example
You can sell token that you want to sell meanwhile you set the type and the id of order bought.

```js
contract: elementscoin
action: transfer
data: {
    "from": "yourself",
    "to": "dexlizenetwk"
    "quantity": "10000.0000 ELE",
    "memo": {"type": "4", "id": "10001"}
}
```

#### Describetion of type in the memo
 * type: 1, the action is the order bought of user
 * type: 2, the action is the order selled of user
 * type: 3, the action is that user want to buy token to cost eos in the order selled
 * type: 4, the action is that user want to get eos by costing token in the order bought