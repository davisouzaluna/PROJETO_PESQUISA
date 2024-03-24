let previousContent = null;

        // Função para carregar documentos de um diretório de mês em um ano específico
        function loadDocumentsForYear(year) {
            const months = ['Janeiro', 'Fevereiro', 'Março', 'Abril', 'Maio', 'Junho', 'Julho', 'Agosto', 'Setembro', 'Outubro', 'Novembro', 'Dezembro'];
            const documentsContainer = document.getElementById('documents');
            documentsContainer.innerHTML = ''; // Limpa o conteúdo anterior
            
            months.forEach(month => {
                fetch(`${year}/${month}/documents.json`) // Carrega o arquivo JSON com os documentos do mês
                    .then(response => response.json())
                    .then(data => {
                        const documents = data.documents;
                        // Verifica se há documentos para o mês atual
                        if (documents.length > 0) {
                            // Adiciona o nome do mês como título
                            documentsContainer.innerHTML += `<h2>${month}</h2>`;
                            // Cria uma lista para os documentos do mês
                            documentsContainer.innerHTML += '<ul>';
                            documents.forEach(document => {
                                // Adiciona um link para o documento e um contêiner para o conteúdo do documento
                                documentsContainer.innerHTML += `<li><a href="#" onclick="loadDocument('${year}/${month}/${document.filename}', this); return false;">${document.name}</a><a href="${year}/${month}/${document.filename}" download class="download-link"><span class="download-text">Download</span></a><div id="content_${year}_${month}_${document.filename}"></div></li>`;
                            });
                            documentsContainer.innerHTML += '</ul>'; // Fecha a lista
                        }
                    })
                    .catch(error => {
                        console.error(`Erro ao carregar documentos do mês ${month} de ${year}:`, error);
                    });
            });
        }

        function loadDocument(filePath, link) {
            const contentId = `content_${filePath.replace(/\//g, '_')}`;
            const contentContainer = document.getElementById(contentId);
        
            if (contentContainer.innerHTML.trim() !== '') {
                contentContainer.innerHTML = ''; // Limpa o conteúdo se já estiver carregado
                return;
            }
        
            fetch(filePath)
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`Erro ao carregar o documento: ${response.statusText}`);
                    }
                    return response.arrayBuffer();
                })
                .then(arrayBuffer => {
                    const uint8Array = new Uint8Array(arrayBuffer);
                    return mammoth.convertToHtml({ arrayBuffer: uint8Array });
                })
                .then(result => {
                    contentContainer.innerHTML = result.value; // Adiciona o HTML convertido ao contêiner
                })
                .catch(error => {
                    console.error('Erro ao carregar o documento:', error);
                });
        }
        