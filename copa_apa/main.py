# Nome do arquivo original
original_filename = '/home/deivison-costa/Área de trabalho/programas/Cpp/APA-Project/copa_apa/n1000m15E_8448.txt'

# Lendo o conteúdo do arquivo original
with open(original_filename, 'r') as file:
    lines = file.readlines()

# Processando cada linha: somando 1 a cada número
modified_lines = []
for line in lines:
    # Remove espaços em branco no início/fim e divide os números
    numbers = line.strip().split()
    # Converte para inteiro, soma 1, converte de volta para string
    modified_numbers = [str(int(num) + 1) for num in numbers]
    # Junta os números modificados em uma linha com espaços
    modified_line = ' '.join(modified_numbers)
    modified_lines.append(modified_line)

# Criando o novo nome do arquivo
new_filename = original_filename.replace('.txt', '_inc.txt')

# Escrevendo as linhas modificadas no novo arquivo
with open(new_filename, 'w') as file:
    file.write('\n'.join(modified_lines))

print(f'Arquivo modificado salvo como: {new_filename}')