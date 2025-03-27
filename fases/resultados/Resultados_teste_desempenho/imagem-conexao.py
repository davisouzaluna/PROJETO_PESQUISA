import matplotlib.pyplot as plt

# Dados fornecidos
protocols = ['QUIC', 'QUIC', 'TCP', 'TCP', 'TLS/TCP', 'TLS/TCP']
intervals = ['50 MS', '100 MS', '50 MS', '100 MS', '50 MS', '100 MS']
times = [0.003014908, 0.003137277, 0.000327549, 0.000051444, 0.011757890, 0.012989165]

# Criar o gráfico
plt.figure(figsize=(10, 6))
for protocol in set(protocols):
    # Filtrar os dados por protocolo
    protocol_times = [times[i] for i in range(len(protocols)) if protocols[i] == protocol]
    protocol_intervals = [intervals[i] for i in range(len(protocols)) if protocols[i] == protocol]
    
    # Plotar os dados
    plt.plot(protocol_intervals, protocol_times, marker='o', label=f'{protocol}')

# Adicionar título e rótulos aos eixos
plt.title('Tempo de Conexão com o Broker por Protocolo e Intervalo')
plt.xlabel('Intervalo de Tempo')
plt.ylabel('Tempo de Conexão (segundos)')
plt.legend()
plt.grid(True)

# Exibir o gráfico
plt.tight_layout()
plt.show()
