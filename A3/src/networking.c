#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef __APPLE__
#include "./endian.h"
#else
#include <endian.h>
#endif

#include "./networking.h"
#include "./sha256.h"

//----------------------------------------
#include <byteswap.h>
#include <time.h>
#include <stdlib.h>

#define min(a,b)(a > b ? b : a)
//----------------------------------------

char server_ip[IP_LEN];
char server_port[PORT_LEN];
char my_ip[IP_LEN];
char my_port[PORT_LEN];

int c;

/*
 * Gets a sha256 hash of specified data, sourcedata. The hash itself is
 * placed into the given variable 'hash'. Any size can be created, but a
 * a normal size for the hash would be given by the global variable
 * 'SHA256_HASH_SIZE', that has been defined in sha256.h
 */
void get_data_sha(const char* sourcedata, hashdata_t hash, uint32_t data_size, int hash_size) {
  SHA256_CTX shactx;
  unsigned char shabuffer[hash_size];
  sha256_init(&shactx);
  sha256_update(&shactx, sourcedata, data_size);
  sha256_final(&shactx, shabuffer);

  for (int i=0; i<hash_size; i++)
  {
    hash[i] = shabuffer[i];
  }
}

/*
 * Gets a sha256 hash of specified data file, sourcefile. The hash itself is
 * placed into the given variable 'hash'. Any size can be created, but a
 * a normal size for the hash would be given by the global variable
 * 'SHA256_HASH_SIZE', that has been defined in sha256.h
 */
void get_file_sha(const char* sourcefile, hashdata_t hash, int size) {
    int casc_file_size;

    FILE* fp = fopen(sourcefile, "rb");
    if (fp == 0)
    {
        printf("Failed to open source: %s\n", sourcefile);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    casc_file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char buffer[casc_file_size];
    fread(buffer, casc_file_size, 1, fp);
    fclose(fp);

    get_data_sha(buffer, hash, casc_file_size, size);
}

//----------------------------------------
void swap_bytes(char* array, int startIndex, int bytesToSwap){
    char swappedBytes[bytesToSwap];
    memset(swappedBytes, 0, bytesToSwap);
    for (int i=0; i<bytesToSwap; i++) {
        swappedBytes[abs(i - (bytesToSwap - 1))] = array[startIndex + i];
    }
    memcpy((void*)&array[startIndex], swappedBytes, bytesToSwap);
}

/*
 * Combine a password and salt together and hash the result to form the 
 * 'signature'. The result should be written to the 'hash' variable. Note that 
 * as handed out, this function is never called. You will need to decide where 
 * it is sensible to do so.
 */
void get_signature(char* password, char* salt, hashdata_t* signature) {
    int n = PASSWORD_LEN + SALT_LEN;
    char sourcedata[n];
    memset(sourcedata, 0, n);
    sprintf(sourcedata, "%s%s", password, salt);
    get_data_sha(sourcedata, *signature, n, SHA256_HASH_SIZE);
}

/*
 * Register a new user with a server by sending the username and signature to 
 * the server.
 * ---OR---
 * Get a file from the server by sending the username and signature, along with
 * a file path. Note that this function should be able to deal with both small 
 * and large files. 
 */
void send_request(int fd, char* header, char* input) {
    uint32_t paySize = min(strlen(input), REQUEST_BODY_LEN);
    int requestSize = REQUEST_HEADER_LEN + paySize;
    char request[requestSize];
    memset(request, 0, requestSize);
    //add Header
    memcpy(&request[0], header, REQUEST_HEADER_LEN);
    //add size of payload and swap bytes
    memcpy(&request[REQUEST_HEADER_LEN - 4], (char*)&paySize, sizeof(uint32_t));
    swap_bytes(request, REQUEST_HEADER_LEN - 4, 4);
    //add payload, up to 128 characters
    memcpy(&request[REQUEST_HEADER_LEN], input, paySize);

    compsys_helper_writen(fd, request, requestSize);    
}

void get_header(char* username, char* password, char* salt, char* header) {
    // 16 bytes - Username, as UTF-8 encoded bytes
    // 32 bytes - Signature, a hash of the salted user password, as UTF-8 encoded bytes
    // 4 bytes - Length of request data, excluding this header, unsigned integer in network byte-order
    memset(header, 0, REQUEST_HEADER_LEN);
    hashdata_t signature;
    get_signature(password, salt, &signature);
    memcpy(&header[0], username, USERNAME_LEN);
    memcpy(&header[USERNAME_LEN], signature, SHA256_HASH_SIZE);
}

int read_block(BlockInfo_t* blockInfo, int fd){
    // HEADER
    // 4 bytes  -   Length of response data, excluding this header,
    //                  unsigned integer in network byte-order
    // 4 bytes  -   Status Code of the response,
    //                  unsigned integer in network byte-order
    // 4 bytes  -   Block number, a zero-based count of which block in
    //                  a potential series of replies this is,
    //                  unsigned integer in network byte-order
    // 4 bytes  -   Block count, the total number of blocks to be sent,
    //                  unsigned integer in network byte-order
    // 32 bytes -   Block hash, a hash of the response data in this
    //                  message only, as UTF-8 encoded bytes
    // 32 bytes -   Total hash, a hash of the total data to be sent
    //                  across all blocks, as UTF-8 encoded bytes
    char header[RESPONSE_HEADER_LEN];
    memset(header,'\0',RESPONSE_HEADER_LEN);
    compsys_helper_readn(fd, &header, RESPONSE_HEADER_LEN);

    //swap bytes 0 - 3 and save as paySize
    swap_bytes(header, 0, 4);
    blockInfo->paySize = *(uint32_t*)&header[0];
    //swap bytes 4 - 7 and save as errorCode
    swap_bytes(header, 4, 4);
    blockInfo->errorCode = *(uint32_t*)&header[4];
    //swap bytes 8 - 11 and save as blockNumber
    swap_bytes(header, 8, 4);
    blockInfo->blockNumber = *(uint32_t*)&header[8];
    //swap bytes 12 - 15 and save as blockCount
    swap_bytes(header, 12, 4);
    blockInfo->blockCount = *(uint32_t*)&header[12];

    memcpy(blockInfo->blockHash, &header[16], SHA256_HASH_SIZE);
    memcpy(blockInfo->totalHash, &header[16+SHA256_HASH_SIZE], SHA256_HASH_SIZE);
    
    blockInfo->payload = malloc(blockInfo->paySize + 1);
    memset(blockInfo->payload, 0, blockInfo->paySize + 1);
    compsys_helper_readn(fd, blockInfo->payload, blockInfo->paySize);

    hashdata_t payloadHash;
    get_data_sha(blockInfo->payload, payloadHash, blockInfo->paySize, SHA256_HASH_SIZE);
    if (*payloadHash == *blockInfo->blockHash) {
        return EXIT_SUCCESS;
    } else {
        printf("Block number %d hash: %x is different from expected hash: %x\n", 
            blockInfo->blockNumber, *payloadHash, *blockInfo->blockHash);
        return EXIT_FAILURE;
    }
    // ERROR CODE
    // 1        -   OK (i.e. no problems encountered)
    // 2        -   User already exists (i.e. could not register a user as
    //                  they are already registerd)
    // 3        -   User missing (i.e. could not service the request as the
    //              user has not yet registered)
    // 4        -   Invalid Login (i.e. the provided signature does not match
    //                  the registerd user)
    // 5        -   Bad Request (i.e. the request is coherent but cannot be
    //                  servered as the file doesn't exist)
    // 6        -   Other (i.e. any error not covered by the other status
    //                  codes)
    // 7        -   Malformed (i.e. the request is malformed and could not be
    //                  processed)
}

int read_all_blocks(BlockInfo_t* blockInfo, int fd) {
    int retVal = read_block(blockInfo, fd);
    if (blockInfo->blockCount > 1 && retVal == EXIT_SUCCESS) {
        char** fullPayload = malloc(sizeof(char*) * blockInfo->blockCount);
        fullPayload[blockInfo->blockNumber] = blockInfo->payload;
        uint32_t blocksRead = 1;
        BlockInfo_t partialInfo;

        while (blocksRead < blockInfo->blockCount) {
            blocksRead++;
            if (read_block(&partialInfo, fd) != EXIT_SUCCESS) {
                retVal = EXIT_FAILURE;
                //Just keep reading the remaining blocks, 
                //as its too big a hasle to figure out what to free if we break here
            }
            fullPayload[partialInfo.blockNumber] = partialInfo.payload;
            blockInfo->paySize += partialInfo.paySize;
        }
        
        blockInfo->payload = malloc(blockInfo->paySize + 1);
        memset(blockInfo->payload, 0, blockInfo->paySize + 1);
        for (uint32_t i = 0; i < blocksRead; i++) {
            strcat(blockInfo->payload, (char*)fullPayload[i]);
            free((char*)fullPayload[i]);
        }
        free((char**)fullPayload);
    }
    return retVal;
}

void write_to_file(char* fileDir, char* fileName, BlockInfo_t blockInfo) {  
    char filePath[strlen(fileDir)+strlen(fileName)];
    sprintf(filePath, "%s%s", fileDir, fileName);
    //create file if does not exist, and write to file
    FILE* fs = fopen(filePath, "w");
    fprintf(fs, "%s", blockInfo.payload);
    fclose(fs);

    //check if hash of data in file is as expected
    hashdata_t payloadHash;
    get_file_sha(filePath, payloadHash, SHA256_HASH_SIZE);
    if (*payloadHash == *blockInfo.totalHash) {
        printf("File retrived successfully\n");
    } else {
        printf("Total hash: %x is different from expected hash: %x\n", 
            *payloadHash, *blockInfo.totalHash);
    }
}
//----------------------------------------

int main(int argc, char **argv) {
    // Users should call this script with a single argument describing what 
    // config to use
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <config file>\n", argv[0]);
        exit(EXIT_FAILURE);
    } 

    // Read in configuration options. Should include a client_directory, 
    // client_ip, client_port, server_ip, and server_port
    char buffer[128];
    fprintf(stderr, "Got config path at: %s\n", argv[1]);
    FILE* fp = fopen(argv[1], "r");
    while (fgets(buffer, 128, fp)) {
        if (starts_with(buffer, CLIENT_IP)) {
            memcpy(my_ip, &buffer[strlen(CLIENT_IP)], 
                strcspn(buffer, "\r\n")-strlen(CLIENT_IP));
            if (!is_valid_ip(my_ip)) {
                fprintf(stderr, ">> Invalid client IP: %s\n", my_ip);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, CLIENT_PORT)) {
            memcpy(my_port, &buffer[strlen(CLIENT_PORT)], 
                strcspn(buffer, "\r\n")-strlen(CLIENT_PORT));
            if (!is_valid_port(my_port)) {
                fprintf(stderr, ">> Invalid client port: %s\n", my_port);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, SERVER_IP)) {
            memcpy(server_ip, &buffer[strlen(SERVER_IP)], 
                strcspn(buffer, "\r\n")-strlen(SERVER_IP));
            if (!is_valid_ip(server_ip)) {
                fprintf(stderr, ">> Invalid server IP: %s\n", server_ip);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, SERVER_PORT)) {
            memcpy(server_port, &buffer[strlen(SERVER_PORT)], 
                strcspn(buffer, "\r\n")-strlen(SERVER_PORT));
            if (!is_valid_port(server_port)) {
                fprintf(stderr, ">> Invalid server port: %s\n", server_port);
                exit(EXIT_FAILURE);
            }
        }        
    }
    fclose(fp);

    fprintf(stdout, "Client at: %s:%s\n", my_ip, my_port);
    fprintf(stdout, "Server at: %s:%s\n", server_ip, server_port);

    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    char user_salt[SALT_LEN+1];

    //----------------------------------------
    char data[] = "../Data/";
    char user[] = "UserInfo/";
    char file[] = "Files/";
    char type[] = ".txt";

    char userDir[strlen(data)+strlen(user)];
    sprintf(userDir, "%s%s", data, user);

    char fileDir[strlen(data)+strlen(file)];
    sprintf(fileDir, "%s%s", data, file);

    int upl = strlen(userDir)+USERNAME_LEN+strlen(type);
    char userPath[upl];
    

    mkdir(data);
    mkdir(userDir);
    mkdir(fileDir);

    FILE *fs;

    char header[REQUEST_HEADER_LEN];
    char fileName[REQUEST_BODY_LEN];

    int fd;
    BlockInfo_t blockInfo;
    int state = 0;


    while (state < 2) {
        switch (state)  {
            case 0: 
                //========================================
                fprintf(stdout, "Enter a username to proceed: ");
                scanf("%16s", username);
                while ((c = getchar()) != '\n' && c != EOF);
                // Clean up username string as otherwise some extra chars can sneak in.
                for (int i=strlen(username); i<USERNAME_LEN; i++)
                {
                    username[i] = '\0';
                }
            
                fprintf(stdout, "Enter your password to proceed: ");
                scanf("%16s", password);
                while ((c = getchar()) != '\n' && c != EOF);
                // Clean up password string as otherwise some extra chars can sneak in.
                for (int i=strlen(password); i<PASSWORD_LEN; i++)
                {
                    password[i] = '\0';
                }
                //========================================

                memset(userPath, 0, upl);
                sprintf(userPath, "%s%s%s", userDir, username, type);
                fs = fopen(userPath, "r");
                
                if (fs != NULL) {
                    memset(user_salt,'\0',SALT_LEN+1);
                    fgets(user_salt, SALT_LEN+1, fs);
                    fclose(fs);
                    fprintf(stdout, "Using old salt:\n");
                } else {
                    //========================================
                    // Note that a random salt should be used, but you may find it easier to
                    // repeatedly test the same user credentials by using the hard coded value
                    // below instead, and commenting out this randomly generating section.
                    srand(time(NULL));//*
                    for (int i=0; i<SALT_LEN; i++)
                    {
                        user_salt[i] = 'a' + (rand() % 26);//*
                    }
                    user_salt[SALT_LEN] = '\0';
                    //========================================

                    fs = fopen(userPath, "w");
                    fprintf(fs, "%s", user_salt);
                    fclose(fs);
                    fprintf(stdout, "Using new salt:\n");
                }
                fprintf(stdout, "%s\n", user_salt);
                
                get_header(username, password, user_salt, header);

                fd = compsys_helper_open_clientfd(server_ip, server_port);
                send_request(fd, header, "");
                read_all_blocks(&blockInfo, fd);
                printf("%s\n", blockInfo.payload);
                free(blockInfo.payload);
                close(fd);
                
                state = 1;
                break;
            
            case 1:
                memset(fileName, 0, REQUEST_BODY_LEN);
                fprintf(stdout, "Enter the file you wish to get, or \"q\" to quit:\n");
                scanf("%s", fileName);
                while ((c = getchar()) != '\n' && c != EOF);
     
                if (strcmp(fileName, "q") == 0) {
                    state = 2;
                } else {
                    fd = compsys_helper_open_clientfd(server_ip, server_port);
                    send_request(fd, header, fileName);
                    int retVal = read_all_blocks(&blockInfo, fd);
                    if (blockInfo.errorCode == 1) {
                        if (retVal == EXIT_SUCCESS) {
                            write_to_file(fileDir, fileName, blockInfo);
                        }              
                    } else {
                        printf("%s\n", blockInfo.payload);
                        if (blockInfo.errorCode == 4) {
                            state = 0;
                        }
                    }
                    free(blockInfo.payload);
                    close(fd);
                }
                break;
        }
    }  
    //----------------------------------------
    exit(EXIT_SUCCESS);
}