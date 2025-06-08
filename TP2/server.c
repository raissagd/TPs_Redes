/*
-----------------------------------------------------
Arquivo server.c referente ao TP1
Implementacao do servidor, ou Unidade Controladora
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

/*
    Descricao: Funcao de comparacao de string
    Entradas: input (string a ser comparada) command (comando que deseja se achar no input)
    Saida: 1 se forem diferentes, 0 se forem iguais
*/
int compareComand(char input[], char command[])  
{  
    if(strlen(input) < strlen(command))
	{
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
    Descricao: Funcao de pegar parametros a partir de uma string
    Entradas: Ponteiros para todas as variaveis e o input (string source)
    Saida:
*/
void pegaParam(int * salaId, int * temp, int * umidade, int * ven1, int * ven2, int * ven3, int * ven4, char * input)
{
    int posAux = 0;
    int phase = 0;

    char param[36];
    bzero(param, 36);

    //Passando por cada char da string input
    for (int i = 0; i < strlen(input); i++)
        {
			//Espaço sinaliza que é para ir pro próximo parâmetro
            if(input[i] == ' ')
			{
                phase++;
                i++;
                posAux = 0;
            }
			
			//O switch escolhe e define qual parâmetro está sendo coletado baseado na fase
			switch (phase)
			{
				case 1: 
                	param[posAux] = input[i];
                	posAux++; 
                	(*salaId) = atoi(param);
					break;
            
            	case 2:
                	param[posAux] = input[i];
                	posAux++; 
                	(*temp) = atoi(param);
					break;

            	case 3:
                	param[posAux] = input[i];
                	posAux++; 
                	(*umidade) = atoi(param);
					break;

            	case 4:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven1) = atoi(param);
					break;
            

            	case 5:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven2) = atoi(param);
					break;
            
				case 6:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven3) = atoi(param);
					break;
            

            	case 7:
                	param[posAux] = input[i];
                	posAux++; 
                	(*ven4) = atoi(param);
					break;
			
				default:
					continue;
			}
        }
}

/*
    Descricao: Funcao de analisar qual comando foi recebido. Dentro dela, ja sao coletados os parâmetros necessários para cada comando
    Entradas: Ponteiros para todas as variavels e o input (string source)
    Saida: Número identificador da ação de cada código
*/
int trataCod (int * salaId, int * temp, int * umidade, int * ven1, int * ven2, int * ven3, int * ven4, char * input)
{
    char comando[36];
    bzero(comando, 36);

	//Coleta do código (primeiros caracteres até o espaço)
    for (int i = 0; i < strlen(input); i++)
    {
        if(input[i] == ' '){
            break;
        }
        comando[i] = input[i];
    }

    if(compareComand (comando, "kill") == 0)
	{
        //Comando kill
        return 0;
    }

    if(compareComand (comando, "CAD_REQ") == 0)
	{
		//Comando Register (Precisa do parâmetro salaID)
        pegaParam(salaId, temp, umidade, ven1, ven2, ven3, ven4, input);
        return 1;
    }

    if(compareComand (comando, "INI_REQ") == 0)
	{
		//Comando Init (Precisa de todos os parâmetros)
        pegaParam(salaId, temp, umidade, ven1, ven2, ven3, ven4, input);
        return 2;
    }

    if(compareComand (comando, "DES_REQ") == 0)
	{
		//Comando Shutdown (Precisa do parâmetro salaID)
        pegaParam(salaId, temp, umidade, ven1, ven2, ven3, ven4, input);
        return 3;
    }

    if(compareComand (comando, "ALT_REQ") == 0)
	{
		//Comando Update (Precisa de todos os parâmetros)
        pegaParam(salaId, temp, umidade, ven1, ven2, ven3, ven4, input);
        return 4;
    }

    if(compareComand (comando, "SAL_REQ") == 0)
	{
		//Comando Load Info (Precisa do parâmetro salaID)
        pegaParam(salaId, temp, umidade, ven1, ven2, ven3, ven4, input);
        return 5;
    }

    if(compareComand (comando, "INF_REQ") == 0)
	{
        //Comando Load Rooms
        return 6;
    }

    printf("Comando nao reconhecido.\n");
    return 7;
}


/*
    Descricao: Struct de uma sala
    Valores (int): ID da Classe, valores dos sensores(Temperatura, Umidade
	Status dos ventiladores 1, 2, 3 e 4)
*/
struct classroom {   
  int classID;           
  int temp, umidade, ven1, ven2, ven3, ven4; 
}classroom;

//Array de salas, no máximo 8 salas (ID's podem variar de 0 a 7)
struct classroom classes[8];

int main(int argc, char *argv[]){

	//Coleta das informações de tipo de IP(v4 eou v6) e Port
	if(argc != 3){
		printf("Informações inválidas. Favor fornecer o tipo de IP e o Port. Encerrando.\n");
	}

    char * versionIp = argv[1];
	int port = atoi(argv[2]);

	//Inicialização de variaveis de parâmetros e controle
    int input, temperatura, umidade, ven1, ven2, ven3, ven4;
    int tamClasses = 0;
    int salaId = 0;
    int valido = 1;
	int cod = 0;

	//Mensagem de resposta do servidor
    char response[500];
    bzero(response, 500);

    char *ipv4 = "127.0.0.1";
    char buffer[500];

    //Cria Socket do Servidor
    int server_socket, client_sock;

    struct sockaddr_in client_addr;
    socklen_t addr_size;

    //Inicialização do socket IPV4
    if(!strcmp(argv[1], "v4")){
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int));

        //Define endereço do servidor
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = inet_addr(ipv4);
        //Bind o socket para o IP e port especificos
        if(bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)))
        {
            printf("Deu erro no bind IPV4\n");
        }
    }

    //Inicialização do socket IPV6
    struct in6_addr address6;
    if(!strcmp(argv[1], "v6")){
        server_socket = socket(AF_INET6, SOCK_STREAM, 0);
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int));

        //Define endereço do servidor
        struct sockaddr_in6 server_address;
        server_address.sin6_family = AF_INET6;
        server_address.sin6_port = htons(atoi(argv[2]));
        server_address.sin6_addr = in6addr_any;
        //Bind o socket para o IP e port específicos
        if(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)))
        {
            printf("Deu erro no bind IPV6\n");
        }
    }

	//Cria Socket do Cliente
    int client_socket;
	int conectou = 0;

	//Enquanto a mensagem do cliente não for kill, continua procurando por conexões e respondendo
    while (1)
    { 
		//Escuta por mensagens
        listen(server_socket, 5);
        printf("[+]Escutando...\n\n");
        //Aceita conexão com o cliente
        addr_size = 0;
        addr_size = sizeof(client_addr);
        client_sock= accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        printf("[+]Cliente conectado.\n");
        conectou = 1;
        
        //Enquanto o cliente não cair, continua respondendo as mensagens
        while (1)
        {
        //Recebe mensagem do cliente
        bzero(buffer, 500);
        recv(client_sock, buffer, sizeof(buffer), 0);
        printf("Cliente: %s\n", buffer);

		//Tratamento da mensagem do cliente com o Switch
		cod = trataCod(&salaId, &temperatura, &umidade, &ven1, &ven2, &ven3, &ven4, buffer);

		//Escreve a resposta do servidor e coloca em response
		switch (cod)
		{
		case 0:
            //Kill - Encerra a conexão com o cliente e finaliza do programa
            strcpy(response, "Encerrando");
            close(client_sock);
            printf("[+]Cliente desconectado.\n\n");
            printf("%s...\n", response);
            return 0;
		case 1:
			//Register - Registro de sala
            valido = 1;
			//Busca do ID na base de dados
            for (int i = 0; i < tamClasses; i++)
            {	
				//Se já existe uma sala com este ID, Error 02
                if(salaId == classes[i].classID){
                    strcpy(response, "ERROR 02");
                    valido = 0;
                }
            }

			//Se não existe sala com o mesmo ID, cria a sala
            if(valido == 1)
			{
                classes[tamClasses].classID = salaId;
                classes[tamClasses].temp = -1;
                classes[tamClasses].umidade = -1;
                classes[tamClasses].ven1 = -1;
                classes[tamClasses].ven2 = -1;
                classes[tamClasses].ven3 = -1;
                classes[tamClasses].ven4 = -1;
                tamClasses++;
                printf("Sala registrada\n");
                strcpy(response, "OK 01");
            }
			break;
		case 2:
            //Init - Inicialização de sala
            valido = 0;
			//Busca do ID na base de dados
            for (int i = 0; i < tamClasses; i++)
            {
				//Sala encontrada
                if(salaId == classes[i].classID)
				{
                    salaId = i;
                    valido = 2;
					
					//Verificação se a sala tem sensores ativos
                    if(classes[salaId].temp == -1){
                        valido = 1;
                    }
					break;
                }
            }

			//Inicialização de sala
            if(valido == 1)
			{
                classes[salaId].temp = temperatura;
                classes[salaId].umidade = umidade;
                classes[salaId].ven1 = ven1;
                classes[salaId].ven2 = ven2;
                classes[salaId].ven3 = ven3;
                classes[salaId].ven4 = ven4;
    
                printf("Sala inicializada\n");
                strcpy(response, "OK 02");
            }

			//Sala não encontrada: Error 03
            if(valido == 0)
			{
                strcpy(response, "ERROR 03");
            }

			//Sensores já instalados: Error 05
            if(valido == 2)
			{
                strcpy(response, "ERROR 05");
            }
			break;
		case 3:
            //Shutdown - Desligamento de sensores de uma sala
            valido = 0;
			//Busca de ID na base de dados
            for (int i = 0; i < tamClasses; i++)
            {
				//Sala encontrada
                if(salaId == classes[i].classID)
				{
                    salaId = i;
                    valido = 1;
					//Checa se os sensores já estão desativados
                    if(classes[i].temp == -1)
					{
                        valido = 2;
                    }
                }
            }

			//Desligamento de sensores
            if(valido == 1)
			{
                classes[salaId].temp = -1;
                classes[salaId].umidade = -1;
                classes[salaId].ven1 = -1;
                classes[salaId].ven2 = -1;
                classes[salaId].ven3 = -1;
                classes[salaId].ven4 = -1;
                printf("Sensores desligados\n");
                strcpy(response, "OK 03");
            }

			//Sala não encontrada: Error 03
            if(valido == 0)
			{
                strcpy(response, "ERROR 03");
            }

			//Sensores não instalados: Error 06
            if(valido == 2)
			{
                strcpy(response, "ERROR 06");
            }
			break;
		case 4:
            //Update - Atualização dos valores de uma sala
            valido = 0;
			//Busca de ID na base de dados
            for (int i = 0; i < tamClasses; i++)
            {
				//Sala encontrada
                if(salaId == classes[i].classID)
				{
                    salaId = i;
                    valido = 1;
					//Checa se os sensores estão desativados
                    if(classes[i].temp == -1)
					{
                        valido = 2;
                    }
                }
            }

			//Atualização de valores
            if(valido == 1)
			{
                classes[salaId].temp = temperatura;
                classes[salaId].umidade = umidade;
                classes[salaId].ven1 = ven1;
                classes[salaId].ven2 = ven2;
                classes[salaId].ven3 = ven3;
                classes[salaId].ven4 = ven4;
                printf("Sala atualizada\n");
                strcpy(response, "OK 04");
            }

			//Sala não encontrada: Erro 03
            if(valido == 0)
			{
                strcpy(response, "ERROR 03\n");
            }

			//Sensores não instalados: Error 06
            if(valido == 2)
			{
                strcpy(response, "ERROR 06\n");
            }
			break;
		case 5:
            //Load Info - Carrega informação de uma sala específica
            valido = 0;
			//Busca de ID na base de dados
            for (int i = 0; i < tamClasses; i++)
            {
				//Sala encontrada
                if(salaId == classes[i].classID)
				{
                    salaId = i;
                    valido = 1;
					//Checa se os sensores não estão instalados
                    if(classes[i].temp == -1)
					{
                        valido = 2;
                    }
                }
            }

			//Carregamento de informações da sala
            if(valido == 1)
			{
                char message[500];
                bzero(message, 500);
                sprintf(message, "sala %d: %d %d %d %d %d %d ", classes[salaId].classID, classes[salaId].temp, 
                                    classes[salaId].umidade, classes[salaId].ven1, classes[salaId].ven2, 
                                    classes[salaId].ven3, classes[salaId].ven4);
                strcpy(response, message);
            }

			//Sala não encontrada: Error 03
            if(valido == 0)
			{
                strcpy(response, "ERROR 03");
            }

			//Sensores não instalados: Error 06
            if(valido == 2)
			{
                strcpy(response, "ERROR 06");
            }
			break;
		case 6:
            //Load Rooms - Carrega todas as informações existentes na base de dados
            char message[100];
            bzero(message, 100);

            char format[36];
            bzero(format, 36);

			//Caso a base não esteja vazia
			if(tamClasses > 0)
			{
				strcat(message, "salas: ");
				//Pega informação de todas as salas
            	for (int i = 0; i < tamClasses; i++)
            	{
                	sprintf(format, "%d (%d %d %d %d %d %d) ", classes[i].classID, classes[i].temp, classes[i].umidade,
                                                         classes[i].ven1, classes[i].ven2, classes[i].ven3, classes[i].ven4);
                	strcat(message, format);
            	}
			}

			//Salas Inexistentes: Error 03
			else
			{
				strcpy(message, "ERROR 03");
			}
            strcpy(response, message);
			break;	
		default:
            //Disconecta e aguarda nova mensagem
            close(client_sock);
            printf("[+]Cliente desconectado.\n\n");
            conectou = 0;
			break;
		}

        //Por algum motivo, o cliente caiu
        if(conectou == 0)
        {
            break;
        }
		//Envia a mensagem de resposta ao cliente
        bzero(buffer, 500);
        strcpy(buffer, response);
        printf("Server: %s\n", buffer);
        send(client_sock, buffer, sizeof(buffer), 0);
        bzero(buffer, 500);
        }
    }
    return 0;
}