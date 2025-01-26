import os
import json

# Gerador de JSON para documentos
# Este script deve ser executado sempre que um novo arquivo ou diretório for adicionado, para atualizar a lista de documentos
def generate_documents_json(month_directory):
    documents = []
    for filename in os.listdir(month_directory):
        if filename.lower().endswith('.docx') or filename.lower().endswith('.pptx'):
            # Obtém o nome do documento removendo a extensão do arquivo
            name = os.path.splitext(filename)[0]
            # Adiciona o documento à lista de documentos
            documents.append({"name": name, "filename": filename})
    return {"documents": documents}

def generate_documents_for_year(year_directory):
    for month in os.listdir(year_directory):
        month_directory = os.path.join(year_directory, month)
        if os.path.isdir(month_directory):
            documents_json = generate_documents_json(month_directory)
            output_file = os.path.join(month_directory, "documents.json")
            with open(output_file, "w") as file:
                json.dump(documents_json, file, indent=4)
            print(f"Arquivo documents.json gerado para {year_directory}/{month}")

# Diretório principal onde os diretórios dos anos estão localizados
main_directory = "../"

# Diretórios dos anos
years_directories = [os.path.join(main_directory, year) for year in os.listdir(main_directory) if os.path.isdir(os.path.join(main_directory, year))]

# Gera os arquivos documents.json para cada ano
for year_directory in years_directories:
    generate_documents_for_year(year_directory)
