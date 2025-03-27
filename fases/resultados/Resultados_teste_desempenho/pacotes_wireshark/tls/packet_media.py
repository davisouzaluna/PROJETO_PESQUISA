import pandas as pd

# Carregue o arquivo CSV
df = pd.read_csv('50ms_tls.csv')

# Verifique os nomes das colunas
print("Nomes das colunas no arquivo CSV:")
print(df.columns)

# Supondo que a coluna com os tamanhos dos pacotes é chamada 'Length'
# Você pode substituir 'Length' pelo nome correto encontrado na etapa acima
tamanhos_pacotes = df['Length']

# Calcule a média
tamanho_medio = tamanhos_pacotes.mean()

print(f"Tamanho médio dos pacotes: {tamanho_medio:.2f} bytes")
