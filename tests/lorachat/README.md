# LoRaChat (SX127x)

Ce sous-projet est un chat LoRa brut (non LoRaWAN) base sur le driver SX127x.

Chaque trame suit le protocole texte:

```
ID-source@salon:numero-message:message
```

Le mode broadcast utilise `*` comme salon:

```
ID-source@*:numero-message:message
```

Par exemple:
* `123@*:11:Qui est la?` : signifie de la part de `123` pour tout le monde (`*`) (message #`11`), Qui est la ? --> doit entrainer une réponse des récepteurs.
* `123@*:12:Salut à tous` : signifie de la part de `123` pour tout le monde (`*`) (message #12)
* `123@frblabla:13:Salut !` : signifie de la part de `123` pour les abonnés du salon `frblabla` (message #`13`)
* `123@456:14:RDV 868100000 SF7BW125` : signifie de la part de `123` à l'utiliteur `456` (message #`14`), RdV sur le canal `868.1` MHz en `SF7` et `BW125`)

## Build

```bash
make
make BOARD=wyres-base DRIVER=sx1272 -j 16
```

## Commandes shell

- `init` : initialise le modem SX127x
- `setup <bw> <sf> <cr>` : regle la modulation LoRa
- `channel set <hz>` : regle la frequence
- `listen` : passe en ecoute continue
- `salon <id>` : choisit le salon d'emission courant
- `send <message>` : envoie une trame `src@salon:numero-message:message` ou `src@*:numero-message:message`
- `salons add <id>` / `salons remove <id>` : s'abonne ou se desabonne d'un salon
- `nodes` : liste les noeuds connus
- `messages` : liste les messages reçus

## Sequence typique

Sur les deux cartes:

```bash
> init
> setup 125 12 5
> channel set 868000000
> listen
```

Sur emetteur (id 12):

```bash
> id set 12
> chat_send 7 bonjour
> chat_send * bonjour_tout_le_monde
```

Sur recepteur (id 7):

```bash
> id set 7
```

Puis la reception affiche le message decode et indique si la trame est destinee a l'ID local.
