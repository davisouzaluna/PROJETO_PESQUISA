# Teste de Latência e análise dos dados

Nesse diretório poderá ser visto o tutorial de execução de como fazer os testes e por último a análise dos dados.

## Como é feito o teste de latência ?

O publisher irá enviar o tempo atual em nanosegundos e o subscriber irá, quando receber a mensagem do publisher, obter o tempo atual e salvar no Redis.


## Teste de latência MQTT/TCP sem TLS


- ### Subscriber (`tcp/sub`)
    1. Verifique se o diretório possui uma pasta `build`, pois é lá onde você vai criar o seu executável, caso nao exista, crie uma pasta com o comando:
        ```bash
        mkdir build && cd build
        ```
    2. Compile o projeto com o comando:
        ```bash
        cmake .. && make
        ```
    3. Lembre de colocar uma chave redis condizente com o que você quer, aqui colocarei **teste_latencia_tcp**;
 
     - Exemplo(depois de você fazer os passos anteriores, `sub/build`):
        ```bash
        ./sub_tcp sub mqtt-tcp://broker.emqx.io:1883 0 teste_latencia_tcp teste_latencia_tcp
        ```

- ### Publisher (`tcp/pub`)
    1. Verifique se o diretório possui uma pasta `build`, pois é lá onde você vai criar o seu executável, caso nao exista, crie uma pasta com o comando:
        ```bash
        mkdir build && cd build
        ```
    2. Compile o projeto com o comando:
        ```bash
        cmake .. && make
        ```
    3. Realize o teste, lembrando de colocar como parâmetro o **intervalo de envio** igual para todos(quic, tcp ou tls). Nesse caso eu deixarei 10 ms. Mas você pode alterar para qualquer outro valor;

    4. Envie pelo menos 10 mil pacotes

    - Exemplo(depois de você fazer os passos anteriores, `pub/build`):
        
        ```bash
        ./pub_tcp mqtt-tcp://broker.emqx.io:1883 0 teste_latencia_tcp 10 10000
        ```

## Teste de Latência MQTT/TCP com TLS

- ### Subscriber(`tcp/sub_tls`)
    1. Verifique se o diretório possui uma pasta `build`, pois é lá onde você vai criar o seu executável, caso nao exista, crie uma pasta com o comando:
        ```bash
        mkdir build && cd build
        ```

    2. Compile o projeto com o comando, lembrando de adicionar a flag para a compilação:
        ```bash
        cmake -DNNG_ENABLE_TLS=ON .. && make
        ```
    3. Lembre de colocar uma chave redis condizente com o que você quer, aqui colocarei **teste_latencia_tls**;
    - Exemplo(depois de você fazer os passos anteriores, `sub_tls/build`):
        ```bash
        ./sub_tls -u tls+mqtt-tcp://broker.emqx.io:8883 -t teste_latencia_tls -q 0 -r teste_latencia_tls
        ```

- ### Publisher(`tcp/pub_tls`)

    1. Verifique se o diretório possui uma pasta `build`, pois é lá onde você vai criar o seu executável, caso nao exista, crie uma pasta com o comando:
        ```bash
        mkdir build && cd build
        ```
    2. Compile o projeto com o comando, lembrando de adicionar a flag para a compilação:
        ```bash
        cmake -DNNG_ENABLE_TLS=ON .. && make
        ```
    3. Realize o teste, lembrando de colocar como parâmetro o **intervalo de envio** igual para todos(quic, tcp ou tls). Nesse caso eu deixarei 10 ms. Mas você pode alterar para qualquer outro valor;

    - Exemplo(depois de você fazer os passos anteriores, `pub_tls/build`):
        ```bash
        ./tls -u tls+mqtt-tcp://broker.emqx.io:8883 -t teste_latencia_tls -q 0 -i 10 -n 10000
        ```

## Teste de latência MQTT/QUIC

- ### Subscriber(`quic/sub`)
   
    1. Verifique se o diretório possui uma pasta `build`, pois é lá onde você vai criar o seu executável, caso nao exista, crie uma pasta com o comando:
        ```bash
        mkdir build && cd build
        ```
    2. Compile o projeto com o comando:
        ```bash
        cmake .. && make
        ```
    3. Lembre de colocar uma chave redis condizente com o que você quer, aqui colocarei **teste_latencia_quic**;
        
    - Exemplo (depois de você fazer os passos anteriores, `sub/build`):
        ```bash
        ./sub sub "mqtt-quic://broker.emqx.io:14567" 0 teste_latencia_quic teste_latencia_quic
        ```


- ### Publisher(`quic/pub`)

    1. Verifique se o diretório possui uma pasta `build`, pois é lá onde você vai criar o seu executável, caso nao exista, crie uma pasta com o comando:
        ```bash
        mkdir build && cd build
        ```
    2. Compile o projeto com o comando:
        ```bash
        cmake .. && make
        ```
    3. Realize o teste, lembrando de colocar como parâmetro o **intervalo de envio** igual para todos(quic, tcp ou tls). Nesse caso eu deixarei 10 ms. Mas você pode alterar para qualquer outro valor.

    - Exemplo(depois de você fazer os passos anteriores, `pub/build`):
        ```bash
        ./publisher mqtt-quic://broker.emqx.io:14567 0 teste_latencia_quic 10000 10
        ```

## Redis-cli

Para acessar o redis na linha de comando apenas digite(lembrando de estar com o serviço do redis ativo):

```bash
redis-cli
```
- ### Features do redis para verificação do teste
    - Comando para saber quais são todas as chaves:
    ```bash
    keys *
    ```
    - Comando para saber quais são todos os dados de uma chave
    ```bash
    lrange <key> 0 -1
    ```

## Como gerar os documentos de análise ?

- Primeiro devemos instalar as dependências necessárias com :
    ```bash
    pip install -r requirements.txt
    ```

- Depois executar o código que irá fazer a análise:
    
    ```bash
    python3 analise.py
    ```
- ### OBS: Os arquivos gerados estão dentro da pasta com o mesmo nome da chave redis.