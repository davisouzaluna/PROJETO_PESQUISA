import os
import json

#gerador de jSON para documentos
#Esse script deve ser executado toda vez que um novo arquivo ou  diretorio for adicionado, para atualizar a lista de documentos
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

# Diretórios dos anos
years_directories = ["reports/pt_br/2023", "reports/pt_br/2024"]

# Gera os arquivos documents.json para cada ano
for year_directory in years_directories:
    year_path = os.path.join(os.getcwd(), year_directory)
    generate_documents_for_year(year_path)
