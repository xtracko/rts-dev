# rts-dev

authors: David Klaška, Tomáš Lamser, Jiří Novotný, Vladimír Štill


To connect to device using ssh on linux:
```
ip addr add 10.42.0.1/24 dev <dev_name>
ssh ev3@10.42.0.3
```
where `<dev_name>` can be determined by use of command `ip addr`.
