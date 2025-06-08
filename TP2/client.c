/*
-----------------------------------------------------
Arquivo client.c referente ao TP1
Implementacao do cliente, ou Unidade de Monitoramento
Materia Redes de Computadores
Semestre 2024/01

Aluna: Maria Clara Oliveira Domingos Ruas
Matricula: 2021019572
-----------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

char cod[500];

/*
    Descricao: Funcao de comparacao de string
    Entradas: input (string a ser comparada) command (comando que deseja se achar no input)
    Saida: 1 se forem diferentes, 0 se forem iguais
*/
int compareComand(char input[], char command[])  
{  
    if(strlen(input) < strlen(command)){
        return 1;
    }
    for (int i = 0; i < strlen(command); i++)
    {
        if (input[i] != command[i])
        {
            return 1;
        }
    }
    return 0;
}  

/*
    Descricao: Envia uma mensagem para o servidor
    Entradas: sock (socket do servidor) message (mensagem a ser enviada)
*/
void sendMessage(int sock, char message[500])
{
    char buffer[500];
    bzero(buffer, 500);

    strcpy(buffer, message);
    send(sock, buffer, strlen(buffer), 0);
}

/*
    Descricao: Funcao de impressao da resposta do servidor
    Entradas: server_response (mensagem de resposta do servidor)
*/
void checkError(char * server_response)
{
	//Descricoes dos erros e confirmacoes
    char * messages[10] = {"sala invalida", "sala ja existe", "sala inexistente", 
                            "sensores invalidos", "sensores ja instalados", "sensores nao instalados",
                            "sala instanciada com sucesso", "sensores inicializados com sucesso",
                            "sensores desligados com sucesso", "informacoes atualizadas com sucesso"};
    int posAux = 0;
    int phase = 0;
    int idResponse = 0;

    char param[36];
    bzero(param, 36);
    
    for (int i = 0; i < strlen(server_response); i++)
        {
            if(server_response[i] == ' ')
			{
                phase++;
                i++;
                posAux = 0;
            }

			//Pega termo "OK" ou "ERROR"
            if(phase == 0)
			{
                param[posAux] = server_response[i];
                posAux++; 
            } 

			//Pega o ID da mensagem
            if(phase == 1)
			{
                param[posAux] = server_response[i];
                posAux++; 
                idResponse = atoi(param);
            }
        }

    //Imprime a descricao do erro
    if(compareComand(server_response, "ERROR") == 0)
	{
        printf("%s\n", messages[idResponse - 1]);
    }

	//Imprime a descricao de confirmacao
    else if(compareComand(server_response, "OK") == 0)
	{
        printf("%s\n", messages[idResponse + 5]);
    }

	//Se a resposta for os dados de Load, imprime a propria resposta
    else
	{
        printf("%s\n", server_response);
    }
}

/*
    Descricao: Funcao de checagem de parametros
    Entradas: valores de temperatura, umidade e vetor de status dos ventiladores
    Saida: 1 se os parâmetros forem validos, 0 caso contrário
*/
int checkParam(int temp, int umidade, int ven[])
{
    //Temperatura invalida
	if(temp < 0 || temp > 40)
	{
        return 0;
    }
	//Umidade invalida
    if(umidade < 0 || umidade > 100)
	{
        return 0;
    }
	//Checagem ventiladores
    for (int i = 0; i < 4; i++)
    {
        int ID = ven[i]/10;
		//Status do ventilador ID invalido\n
        if(ven[i]%10 > 2 || ven[i]%10 < 0)
		{
            return 0;
        }
        
		//ID do ventilador invalido
        if(ID > 4 || ID < 1)
		{
            return 0;
        }

		//ID na posicao errada
        if (i + 1 != ID)
        {
            return 0;
        }  
    }
    return 1;
}

/*
    Descricao: Funcao de checagem do comando
    Entradas: string input
    Saida: código do tipo de comando
*/
int tipoComando(char * input)
{
    char comando[36];
    bzero(comando, 36);
	//Separa o primeiro termo do resto da string
    for (int i = 0; i < strlen(input); i++)
    {
        if(input[i] == ' '){
            break;
        }
        comando[i] = input[i];
    }
	
	//Comando kill
    if(compareComand (comando, "kill") == 0)
	{
		return 0;
	}

	//Comando Register
    if(compareComand (comando, "register") == 0)
	{
		return 1;
	}

	//Comando Init
    if(compareComand (comando, "init") == 0)
	{
		return 2;
	}

	//Comando Shutdown
    if(compareComand (comando, "shutdown") == 0)
	{
		return 3;
	}
    
	//Comando Update
    if(compareComand (comando, "update") == 0)
	{
		return 4;
	}
    
	//Comando Load
    if(compareComand (comando, "load") == 0)
	{
        return 5;
    }

	//Comando não reconhecido
    return 6;
}

/*
    Descricao: Funcao de tratamento de funcoes de Registro e Shutdown
    Entradas: string input, type (selecao entre registro ou shutdown), network_socket(socket do servidor) server_address (endereco do server)
    Saida: código de sucesso ou erro
*/
int tratamentoRegistro(char * input, int type, int network_socket)
{
    int salaId;
	int phase = 0;
	int posAux = 0;

	char param[36];
    bzero(param, 36);

	//Coleta o ID da sala
	for (int i = 0; i < strlen(input); i++)
    {
        if(input[i] == ' ')
		{
            phase++;
            i++;
        }
        if(phase == 1)
		{
            param[posAux] = input[i];
            posAux++; 
            salaId = atoi(param);
        }

        if(phase > 1)
		{
            return 0;
        }

    }

	//Envio da mensagem  
    char message[500];
    bzero(message, 500);

	//Constroi a mensagem baseada no type (Register:0, Shutdown:1)
    if(type == 0)
	{
    //Checa se o ID é valido
        if(salaId < 0 || salaId > 7)
	    {
            return 0;
        }
        strcat(message, "CAD_REQ x");
        message[8] = salaId + '0';
    }

    if(type == 1)
	{
        sprintf(message, "DES_REQ %d", salaId);
    }

	//Escreve a mensagem em cod e retorna sucesso
    strcpy(cod, message);
    
    return 1;   
}

/*
    Descricao: Funcao de pegar parametros a partir de uma string
    Entradas: Ponteiros para todas as variaveis e o input (string source)
    Saida: código garantindo que pegou todos os parâmetros
*/
int pegaParam(int * salaId, int * temp, int * umidade, int * ven1, int * ven2, int * ven3, int * ven4, int * extra, char * input)
{
    int posAux = 0;
    int phase = 0;

	int final = 0;

    char param[36];
    bzero(param, 36);
    
	//Passa por toda a string
    for (int i = 0; i < strlen(input); i++)
        {
			//A cada ' ', novo parâmetro é pego
            if(input[i] == ' ')
			{
                phase++;
                i++;
                posAux = 0;
            }

			//Switch adiciona o valor da string na variável correspondente
            switch (phase)
			{
				//ID da sala
				case 2: 
                	param[posAux] = input[i];
                	posAux++; 
                	(*salaId) = atoi(param);
					break;

				//Temperatura
            	case 3:
                	param[posAux] = input[i];
                	posAux++; 
                	(*temp) = atoi(param);
					break;

				//Umidade
            	case 4:
                	param[posAux] = input[i];
                	posAux++; 
                	(*umidade) = atoi(param);
					break;

				//Status ventilador 1
            	case 5:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven1) = atoi(param);
					break;
            
				//Status ventilador 2
            	case 6:
                	param[posAux] = input[i];
                	posAux++; 
					(*ven2) = atoi(param);
					break;
            
				//Status ventilador 3
				case 7:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven3) = atoi(param);
					break;
            
				//Status ventilador 4
            	case 8:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven4) = atoi(param);
					//Todos os parâmetros coletados
					final = 1;
					break;
                //Variável a mais
            	case 9:
                	param[posAux] = input[i];
                	posAux++; 
                	(*extra) = atoi(param);
					break;
				default:
					continue;
			}
        }
	//Se tiver coletado todos os dados, é verdade. Se negativo, é falso
	return final;
}

/*
    Descricao: Funcao de tratamento de funcoes de Init e Update
    Entradas: string input, type (selecao entre init ou update), network_socket(socket do servidor) server_address (endereco do server)
    Saida: código de sucesso ou erro (ID da sala invalido ou parametros invalidos)
*/
int tratamentoInit(char * input, int type, int network_socket)
{
    int phase = 0;
    int posAux = 0;
    int extra = -1;

    int salaId;

    char source[36];
    bzero(source, 36);

    char fileName[36];
    bzero(fileName, 36);

    char param[36];
    bzero(param, 36);

    
    int temp, umidade, ven1, ven2, ven3, ven4; 

	//Separação do source das informações
    for (int i = 0; i < strlen(input); i++)
    {
        if(input[i] == ' ')
		{
            phase++;
            i++;
        }
        if(phase == 1)
		{
            source[posAux] = input[i];
            posAux++; 
        }
    }

    phase = 0;
    posAux = 0;

	//Coleta informações do arquivo
    if(compareComand (source, "file") == 0)
	{
        FILE *fptr;
		//Coleta o nome do arquivo
        for (int i = 0; i < strlen(input); i++)
        {
            if(input[i] == ' ' || input[i] == '\n')
			{
                phase++;
                i++;
            }
            if(phase == 2)
			{
                fileName[posAux] = input[i];
                posAux++; 
            }
        }
		//Abre o arquivo para leitura
        fptr = fopen(fileName, "r");
        if(fptr == NULL)
        {
            return 0;             
        }

		//Coleta os parametros dentro do arquivo em uma linha
        fscanf(fptr,"%d %d %d %d %d %d %d %d", &salaId, &temp, &umidade, &ven1, &ven2, &ven3, &ven4, &extra);
        fclose(fptr);
    }

	//Coleta informações da string de input
    if(compareComand (source, "info") == 0)
	{
		int param = pegaParam(&salaId, &temp, &umidade, &ven1, &ven2, &ven3, &ven4, &extra, input);
		//Parâmetros incompletos
		if(param == 0)
		{
			return 1;
		}
    }

    if(extra != -1)
    {
        return 1;
    }

	//ID da sala invalido
    if(salaId < 0 || salaId > 7)
	{
        return 0;
    }

    int ven[4] = {ven1, ven2, ven3, ven4};
	//Checa validade dos parametros
    if(checkParam(temp, umidade, ven) == 0)
	{
        return 1;
    }

	//Envio da mensagem
    else
	{
        char format[36];
        bzero(format, 36);
        char message[100];
        bzero(message, 100);

		//Constroi a mensagem baseada no type (Init:0, Update:1)
        if(type == 0)
		{
            strcat(message, "INI_REQ");
        }
        if(type == 1)
		{
            strcat(message, "ALT_REQ");
        }
        
        int par[7] = {salaId, temp, umidade, ven1, ven2, ven3, ven4};
        for (int i = 0; i < 7; i++)
        {
            sprintf(format, "%d", par[i]);
            strcat(message, " "); 
            strcat(message, format);  
        }

		//Escreve mensagem em cod
        strcpy(cod, message);
        return 2;
    }
}

/*
    Descricao: Funcao de tratamento de funcoes de Load Info e Load Rooms
    Entradas: string input, network_socket(socket do servidor) server_address (endereco do server)
    Saida: código de sucesso ou erro (ID da sala invalido)
*/
int tratamentoLoad(char * input, int network_socket){
    int phase = 0;
    int posAux = 0;

    int salaId;

    char source[36];
    bzero(source, 36);

    char param[36];
    bzero(param, 36);

	//Seleciona segundo parâmetro do load (info ou rooms)
    for (int i = 0; i < strlen(input); i++)
    {
        if(input[i] == ' ')
		{
            phase++;
            i++;
        }
        if(phase == 1)
		{
            source[posAux] = input[i];
            posAux++; 
        }
    }

	//Load rooms
    if(compareComand (source, "rooms") == 0)
	{
		//Escreve a mensagem
        char message[] = "INF_REQ";
        strcpy(cod, message);
		return 1;
    }

	//Load Info
    if(compareComand (source, "info") == 0)
	{
		int salaId;
		phase = 0;
		posAux = 0;

		char param[36];
    	bzero(param, 36);

		//Coleta o ID da sala
		for (int i = 0; i < strlen(input); i++)
    	{
        	if(input[i] == ' ')
			{
            	phase++;
            	i++;
        	}
        	if(phase == 2)
			{
            	param[posAux] = input[i];
            	posAux++; 
            	salaId = atoi(param);
        	}
    	}
		
		//Escreve a mensagem
        char message[500];
        sprintf(message, "SAL_REQ %d", salaId);
        strcpy(cod, message);
		return 1;
    }
}


int main(int argc, char *argv[]){

	//Coleta das informações do IP e do Port
	if(argc != 3){
		printf("Informações inválidas. Favor fornecer o IP e o Port. Encerrando.\n");
	}

    char *ip = argv[1];
	int port = atoi(argv[2]);
    
    //Cria um socket
    int network_socket;
    int protocol = 0;

    struct in_addr ipv4;
    struct in6_addr ipv6;

    //Checa se o IP passado pelo argv é IPV4. Se positivo, cria o socket IPV4
    struct sockaddr_in server_address4;
    if(inet_pton(AF_INET, ip, &ipv4))
    {
        protocol = 4;
        //Especifica um endereco para o socket
        server_address4.sin_family = AF_INET;
        server_address4.sin_port = htons(port);
        server_address4.sin_addr.s_addr = inet_addr(ip);
        //Inicializa o socket
        network_socket = socket(AF_INET, SOCK_STREAM, 0);
        int connection_status = connect(network_socket, (struct sockaddr*) &server_address4, sizeof(server_address4));
        //Checa por erros na conexão
        if(connection_status == -1)
	    {
            printf("Houve um erro no estabelecimento da conexao remota socket \n\n");
        }
    }

    //Checa se o IP passado pelo argv é IPV6. Se positivo, cria o socket IPV6
    struct sockaddr_in6 server_address6;
    if(inet_pton(AF_INET6, ip, &ipv6))
    {
        protocol = 6;
        //Especifica um endereco para o socket
        server_address6.sin6_family = AF_INET6;
        server_address6.sin6_port = htons(port);
        server_address6.sin6_addr = ipv6;
        //Inicializa o socket
        network_socket = socket(AF_INET6, SOCK_STREAM, 0);
        int connection_status = connect(network_socket, (struct sockaddr_in6 *) &server_address6, sizeof(server_address6));
        //Checa por erros na conexão
        if(connection_status == -1)
	    {
            printf("Houve um erro no estabelecimento da conexao remota socket \n\n");
        }
    }

	// Declarando um array temp para armazenar o input de cada linha
	char temp[500]; 
	char buffer[500];
   
    int check = 1;
	int command = 0;
    int envia = 0;

   // Pegando user input no loop
   while (fgets(temp, 200, stdin))
   {
		//Seleciona o tipo de comando
		command = tipoComando(temp);
        envia = 0;
        bzero(cod, 500);

		//Analisa input, envia o codigo para o servidor quando válido, e analisa as mensagens de erro ou confirmação
		switch (command)
		{
		case 0:
			//Kill - Encerra o servidor e o cliente
            strcat(cod, "kill");
            sendMessage(network_socket, cod);
            close(network_socket);
			return 0;

		case 1:
			//Register
            check = tratamentoRegistro(temp, 0, network_socket);
            //ID invalido: Error 01
			if(check == 0)
			{
				checkError("ERROR 01");
            }
            //Recebe resposta do servidor
			else
			{
                envia = 1;
            }
			break;

		case 2:
			//Init
            check = tratamentoInit(temp, 0, network_socket);
			//ID invalido: Error 01
            if(check == 0)
			{
				checkError("ERROR 01");
            }
            //Parâmetros inválidos: Error 04
			else if(check == 1)
			{
				checkError("ERROR 04");
            }
            //Recebe resposta do servidor
			else{
                envia = 1;
            }
			break;

		case 3:
			//Shutdown
            check = tratamentoRegistro(temp, 1, network_socket);
			//Sala inexistente: Error 03
			if(check == 0)
			{
				checkError("ERROR 03");
            }
			//Recebe resposta do servidor
            else
			{
                envia = 1;
            }
			break;

		case 4:
			//Update
            check = tratamentoInit(temp, 1, network_socket);
			//ID invalido: Error 01
            if(check == 0)
			{
				checkError("ERROR 01");
            }
			//Sensores invalidos: Error 04
            else if(check == 1)
			{
				checkError("ERROR 04");
            }
			//Recebe resposta do servidor
            else
			{
                envia = 1;
            }
			break;
		
		case 5:
			//Load Info ou Load Rooms
            check = tratamentoLoad(temp, network_socket);
			//ID invalido: Error 01
			if(check == 0)
			{
				checkError("ERROR 01");
            }
			//Recebe resposta do servidor
            else
			{
                envia = 1;
            }
			break;
		default:
			break;
		}

		//Comando nao reconhecido. Encerrar o cliente
        if(tipoComando(temp) == 6)
		{
            //printf("Comando nao reconhecido. Encerrando.\n");
            break;
        }

        if(envia == 1){
            bzero(buffer, 500);
            send(network_socket, cod, strlen(cod), 0);
            recv(network_socket, buffer, sizeof(buffer), 0);
            checkError(buffer);
        }
   }

   close(network_socket);

    return 0;
}