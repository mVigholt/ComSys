#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include "./endian.h"
#else
#include <endian.h>
#endif

#include "./peer.h"
#include "./sha256.h"


// Global variables to be used by both the server and client side of the peer.
// Some of these are not currently used but should be considered STRONG hints
PeerAddress_t *my_address;

pthread_mutex_t network_mutex = PTHREAD_MUTEX_INITIALIZER;
PeerAddress_t** network = NULL;
uint32_t peer_count = 0;

pthread_mutex_t retrieving_mutex = PTHREAD_MUTEX_INITIALIZER;
char filePath[100] = "Data/";
FilePath_t** retrieving_files = NULL;
uint32_t file_count = 0;

//----------------------------------------
void swap_bytes(char* array, int startIndex, int bytesToSwap){
    char swappedBytes[bytesToSwap];
    memset(swappedBytes, 0, bytesToSwap);
    for (int i=0; i<bytesToSwap; i++) {
        swappedBytes[abs(i - (bytesToSwap - 1))] = array[startIndex + i];
    }
    memcpy((void*)&array[startIndex], swappedBytes, bytesToSwap);
}


int cmp_PeerAddress(PeerAddress_t peerAddress1, PeerAddress_t peerAddress2){
    return (strncmp(peerAddress1.ip, peerAddress2.ip, IP_LEN) != 0 || 
                        strncmp(peerAddress1.port, peerAddress2.port, PORT_LEN) != 0 );
}


int add_peer(Address_t client_address)
{
    pthread_mutex_lock(&network_mutex);
    for (uint32_t i = 0; i < peer_count; i++){
        if (cmp_PeerAddress(*network[i], client_address.peer) == 0) {
            pthread_mutex_unlock(&network_mutex);
            return EXIT_FAILURE;
        }
    }
    
    network = realloc(network, (peer_count + 1) * sizeof(PeerAddress_t*));
    network[peer_count] = (PeerAddress_t*)malloc(sizeof(PeerAddress_t));
    memset(network[peer_count]->ip, 0, IP_LEN);
    memcpy(network[peer_count]->ip, client_address.peer.ip, IP_LEN);
    memset(network[peer_count]->port, 0, PORT_LEN);
    memcpy(network[peer_count]->port, client_address.peer.port, PORT_LEN);
    printf("Peer added:\nIp: %s, Port: %s\n", network[peer_count]->ip, network[peer_count]->port);
    peer_count ++;
    pthread_mutex_unlock(&network_mutex);
    return EXIT_SUCCESS;
}
//----------------------------------------

/*
 * Gets a sha256 hash of specified data, sourcedata. The hash itself is
 * placed into the given variable 'hash'. Any size can be created, but a
 * a normal size for the hash would be given by the global variable
 * 'SHA256_HASH_SIZE', that has been defined in sha256.h
 */
void get_data_sha(const char* sourcedata, hashdata_t hash, uint32_t data_size, 
    int hash_size)
{
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
void get_file_sha(const char* sourcefile, hashdata_t hash, int size)
{
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

/*
 * A simple min function, which apparently C doesn't have as standard
 */
uint32_t min(int a, int b)
{
    if (a < b) 
    {
        return a;
    }
    return b;
}

/*
 * Select a peer from the network at random, without picking the peer defined
 * in my_address
 */
void get_random_peer(PeerAddress_t* peer_address)
{ 
    PeerAddress_t** potential_peers = malloc(sizeof(PeerAddress_t*));
    uint32_t potential_count = 0; 
    pthread_mutex_lock(&network_mutex);
    for (uint32_t i=0; i<peer_count; i++)
    {   
        if (strncmp(network[i]->ip, my_address->ip, 16) != 0 
                || strncmp(network[i]->port, my_address->port, 8) != 0 )
        {   
            potential_peers = realloc(potential_peers, 
                (potential_count+1) * sizeof(PeerAddress_t*));
            potential_peers[potential_count] = network[i];
            potential_count++;
        }
    }
    pthread_mutex_unlock(&network_mutex);

    if (potential_count == 0)
    {
        printf("No peers to connect to. You probably have not implemented "
            "registering with the network yet.\n");
    }

    uint32_t random_peer_index = rand() % potential_count;
    
    memcpy(peer_address->ip, potential_peers[random_peer_index]->ip, IP_LEN);
    memcpy(peer_address->port, potential_peers[random_peer_index]->port, 
        PORT_LEN);

    free(potential_peers);
    
    printf("Selected random peer: %s:%s\n", 
        peer_address->ip, peer_address->port);
}

/*
 * Send a request message to another peer on the network. Unless this is 
 * specifically an 'inform' message as described in the assignment handout, a 
 * reply will always be expected.
 */
// void send_message(PeerAddress_t peer_address, int command, char* request_body) {
void send_message(PeerAddress_t peer_address, int command, char* request_body, uint32_t request_body_len) {
    fprintf(stdout, "Connecting to server at %s:%s to run command %d (%s)\n", 
        peer_address.ip, peer_address.port, command, request_body);

    compsys_helper_state_t state;
    char msg_buf[MAX_MSG_LEN];
    FILE* fp;

    // Setup the eventual output file path. This is being done early so if 
    // something does go wrong at this stage we can avoid all that pesky 
    // networking
    //char output_file_path[strlen(request_body)+1];
    //--------------------------------------------------
    char output_file_path[strlen(filePath)+request_body_len+1];
    //--------------------------------------------------
    if (command == COMMAND_RETREIVE)
    {     
        //strcpy(output_file_path, request_body);
        //--------------------------------------------------
        strcpy(output_file_path, filePath);
        strcat(output_file_path, request_body);
        //--------------------------------------------------
        // if (access(output_file_path, F_OK ) != 0 ) 
        // {
        //     fp = fopen(output_file_path, "a");
        //     fclose(fp);
        // }
    }

    // Setup connection
    int peer_socket = compsys_helper_open_clientfd(peer_address.ip, peer_address.port);
    compsys_helper_readinitb(&state, peer_socket);

    // Construct a request message and send it to the peer
    struct RequestHeader request_header;
    strncpy(request_header.ip, my_address->ip, IP_LEN);
    request_header.port = htonl(atoi(my_address->port));
    request_header.command = htonl(command);
    //request_header.length = htonl(strlen(request_body));
    //--------------------------------------------------
    request_header.length = htonl(request_body_len);
    //--------------------------------------------------

    memcpy(msg_buf, &request_header, REQUEST_HEADER_LEN);
    //memcpy(msg_buf+REQUEST_HEADER_LEN, request_body, strlen(request_body));
    //--------------------------------------------------
    memcpy(msg_buf+REQUEST_HEADER_LEN, request_body, request_body_len);
    //--------------------------------------------------

    // compsys_helper_writen(peer_socket, msg_buf, REQUEST_HEADER_LEN+strlen(request_body));
    //--------------------------------------------------
    compsys_helper_writen(peer_socket, msg_buf, REQUEST_HEADER_LEN+request_body_len);
    //--------------------------------------------------

    // We don't expect replies to inform messages so we're done here
    if (command == COMMAND_INFORM)
    {
        return;
    }

    // Read a reply
    compsys_helper_readnb(&state, msg_buf, REPLY_HEADER_LEN);
    // Extract the reply header 
    char reply_header[REPLY_HEADER_LEN];
    memcpy(reply_header, msg_buf, REPLY_HEADER_LEN);

    uint32_t reply_length = ntohl(*(uint32_t*)&reply_header[0]);
    uint32_t reply_status = ntohl(*(uint32_t*)&reply_header[4]);
    uint32_t this_block = ntohl(*(uint32_t*)&reply_header[8]);
    uint32_t block_count = ntohl(*(uint32_t*)&reply_header[12]);
    hashdata_t block_hash;
    memcpy(block_hash, &reply_header[16], SHA256_HASH_SIZE);
    hashdata_t total_hash;
    memcpy(total_hash, &reply_header[48], SHA256_HASH_SIZE);

    // Determine how many blocks we are about to recieve
    hashdata_t ref_hash;
    memcpy(ref_hash, &total_hash, SHA256_HASH_SIZE);
    uint32_t ref_count = block_count;

    //--------------------------------------------------
    if (command == COMMAND_RETREIVE) {
        pthread_mutex_lock(&retrieving_mutex);
            
        if (reply_status == STATUS_OK)
        {     
            if (access(output_file_path, F_OK ) != 0 ) 
            {
                fp = fopen(output_file_path, "a");
                fclose(fp);
            }
        }
    }
    //--------------------------------------------------

    // Loop until all blocks have been recieved
    for (uint32_t b=0; b<ref_count; b++)
    {
        // Don't need to re-read the first block
        if (b > 0)
        {
            // Read the response
            compsys_helper_readnb(&state, msg_buf, REPLY_HEADER_LEN);

            // Read header
            memcpy(reply_header, msg_buf, REPLY_HEADER_LEN);

            // Parse the attributes
            reply_length = ntohl(*(uint32_t*)&reply_header[0]);
            reply_status = ntohl(*(uint32_t*)&reply_header[4]);
            this_block = ntohl(*(uint32_t*)&reply_header[8]);
            block_count = ntohl(*(uint32_t*)&reply_header[12]);

            memcpy(block_hash, &reply_header[16], SHA256_HASH_SIZE);
            memcpy(total_hash, &reply_header[48], SHA256_HASH_SIZE);

            // Check we're getting consistent results
            if (ref_count != block_count)
            {
                fprintf(stdout, 
                    "Got inconsistent block counts between blocks\n");
                close(peer_socket);
                //--------------------------------------------------
                pthread_mutex_unlock(&retrieving_mutex);
                //--------------------------------------------------
                return;
            }

            for (int i=0; i<SHA256_HASH_SIZE; i++)
            {
                if (ref_hash[i] != total_hash[i])
                {
                    fprintf(stdout, 
                        "Got inconsistent total hashes between blocks\n");
                    close(peer_socket);
                    //--------------------------------------------------
                    pthread_mutex_unlock(&retrieving_mutex);
                    //--------------------------------------------------
                    return;
                }
            }
        }

        // Check response status
        if (reply_status != STATUS_OK)
        {
            if (command == COMMAND_REGISTER && reply_status == STATUS_PEER_EXISTS)
            {
                printf("Peer already exists\n");
            }
            //--------------------------------------------------
            else if (reply_status > STATUS_OK && reply_status <= STATUS_MALFORMED)
            {
                // Read the payload
                char payload[reply_length+1];
                compsys_helper_readnb(&state, msg_buf, reply_length);
                memcpy(payload, msg_buf, reply_length);
                payload[reply_length] = '\0';
                printf("%s\n", payload);
                pthread_mutex_unlock(&retrieving_mutex);
                return;
            }
            //--------------------------------------------------
            else
            {
                printf("Got unexpected status %d\n", reply_status);
                close(peer_socket);
                //--------------------------------------------------
                pthread_mutex_unlock(&retrieving_mutex);
                //--------------------------------------------------
                return;
            }
        }

        // Read the payload
        char payload[reply_length+1];
        compsys_helper_readnb(&state, msg_buf, reply_length);
        memcpy(payload, msg_buf, reply_length);
        payload[reply_length] = '\0';
        
        // Check the hash of the data is as expected
        hashdata_t payload_hash;
        get_data_sha(payload, payload_hash, reply_length, SHA256_HASH_SIZE);

        for (int i=0; i<SHA256_HASH_SIZE; i++)
        {
            if (payload_hash[i] != block_hash[i])
            {
                fprintf(stdout, "Payload hash does not match specified\n");
                close(peer_socket);
                //--------------------------------------------------
                pthread_mutex_unlock(&retrieving_mutex);
                //--------------------------------------------------
                return;
            }
        }
        
        // If we're trying to get a file, actually write that file
        if (command == COMMAND_RETREIVE)
        {
            // Check we can access the output file
            fp = fopen(output_file_path, "r+b");

            if (fp == 0)
            {
                printf("Failed to open destination: %s\n", output_file_path);
                close(peer_socket);
            }

            uint32_t offset = this_block * (MAX_MSG_LEN-REPLY_HEADER_LEN);
            fprintf(stdout, "Block num: %d/%d (offset: %d)\n", this_block+1, 
                block_count, offset);
            fprintf(stdout, "Writing from %d to %d\n", offset, 
                offset+reply_length);

            // Write data to the output file, at the appropriate place
            fseek(fp, offset, SEEK_SET);
            fputs(payload, fp);
            fclose(fp);
        }
    }

    // Confirm that our file is indeed correct
    if (command == COMMAND_RETREIVE)
    {
        fprintf(stdout, "Got data and wrote to %s\n", output_file_path);

        // Finally, check that the hash of all the data is as expected
        hashdata_t file_hash;
        get_file_sha(output_file_path, file_hash, SHA256_HASH_SIZE);

        for (int i=0; i<SHA256_HASH_SIZE; i++)
        {
            if (file_hash[i] != total_hash[i])
            {
                fprintf(stdout, "File hash does not match specified for %s\n", 
                    output_file_path);
                close(peer_socket);
                //--------------------------------------------------
                pthread_mutex_unlock(&retrieving_mutex);
                //--------------------------------------------------
                return;
            }
        }
        //--------------------------------------------------
        pthread_mutex_unlock(&retrieving_mutex);
        //--------------------------------------------------
    }

    // If we are registering with the network we should note the complete 
    // network reply
    char* reply_body = malloc(reply_length + 1);
    memset(reply_body, 0, reply_length + 1);
    memcpy(reply_body, msg_buf, reply_length);

    if (reply_status == STATUS_OK)
    {
        if (command == COMMAND_REGISTER)
        {   
            // Your code here. This code has been added as a guide, but feel 
            // free to add more, or work in other parts of the code
            //--------------------------------------------------        
            Address_t address;
            uint32_t peerCount = reply_length/20;
            for (uint32_t i = 0; i < peerCount; i++) {
                memset(address.peer.ip, 0, IP_LEN);
                memset(address.peer.port, 0, PORT_LEN);
                memcpy(address.peer.ip, &reply_body[i*20], 16);
                swap_bytes(reply_body, (i*20)+16, 4);
                address.port = *(uint32_t*)&reply_body[(i*20)+16];
                sprintf(address.peer.port, "%u", address.port);
                add_peer(address);
            }
            //--------------------------------------------------
        }
    } 
    else
    {
        printf("Got response code: %d, %s\n", reply_status, reply_body);
    }
    free(reply_body);
    close(peer_socket);
}


/*
 * Function to act as thread for all required client interactions. This thread 
 * will be run concurrently with the server_thread but is finite in nature.
 * 
 * This is just to register with a network, then download two files from a 
 * random peer on that network. As in A3, you are allowed to use a more 
 * user-friendly setup with user interaction for what files to retrieve if 
 * preferred, this is merely presented as a convienient setup for meeting the 
 * assignment tasks
 */ 
void* client_thread(void* thread_args)
{
    struct PeerAddress *peer_address = thread_args;

    // Register the given user
    send_message(*peer_address, COMMAND_REGISTER, "\0", 0);
    
    // Update peer_address with random peer from network
    get_random_peer(peer_address);

    // Retrieve the smaller file, that doesn't not require support for blocks
    send_message(*peer_address, COMMAND_RETREIVE, "tiny.txt", strlen("tiny.txt"));

    // Update peer_address with random peer from network
    get_random_peer(peer_address);

    // Retrieve the larger file, that requires support for blocked messages
    send_message(*peer_address, COMMAND_RETREIVE, "hamlet.txt", strlen("hamlet.txt"));

    // Update peer_address with random peer from network
    get_random_peer(peer_address);

    // Retrieve the larger file, that requires support for blocked messages
    send_message(*peer_address, COMMAND_RETREIVE, "doesNotExist.txt", strlen("doesNotExist.txt"));
    
    // Update peer_address with random peer from network
    get_random_peer(peer_address);

    // Retrieve the larger file, that requires support for blocked messages
    send_message(*peer_address, COMMAND_RETREIVE, "empty.txt", strlen("empty.txt"));


    return NULL;
}

//--------------------------------------------------
void handle_reply(int connfd, Reply_t reply) {
    char errorBody[MAX_MSG_LEN-REPLY_HEADER_LEN] = {'\0'};
    
    if (reply.header.status == STATUS_OK) {
        get_data_sha(reply.body, reply.header.total_hash, reply.header.length, SHA256_HASH_SIZE);
    } else {
        switch (reply.header.status){
            case STATUS_PEER_EXISTS:
                strcat(errorBody,"Peer already exists (i.e. could not register a peer as they are already registerd with the network)");
                break;
            case STATUS_BAD_REQUEST:
                strcat(errorBody,"Bad Request (i.e. the request is coherent but cannot be servered as the file doesn't exist or is busy)");
                break;
            case STATUS_OTHER:
                strcat(errorBody,"Other (i.e. any error not covered by the other status codes)");
                break;
            case STATUS_MALFORMED:
                strcat(errorBody,"Malformed (i.e. the request is malformed and could not be processed)");
                break;
            default:
                reply.header.status = STATUS_OTHER;
                strcat(errorBody,"Other (i.e. any error not covered by the other status codes)");
                break;
            }
        reply.body = errorBody;
        reply.header.length = strlen(errorBody);
        get_data_sha(errorBody, reply.header.total_hash, reply.header.length, SHA256_HASH_SIZE);
    }
    
    if (reply.header.length > 0) {
        reply.header.block_count = (reply.header.length / (MAX_MSG_LEN-REPLY_HEADER_LEN)) + 
                                min(reply.header.length % (MAX_MSG_LEN-REPLY_HEADER_LEN), 1);
    } else {
        reply.header.block_count = 1;
    }
    
    char block[MAX_MSG_LEN+1];
    for (u_int32_t i = 0; i < reply.header.block_count; i++) {
        memset(block, 0, MAX_MSG_LEN+1);
        u_int32_t bodyLen = min(reply.header.length-(i*(MAX_MSG_LEN-REPLY_HEADER_LEN)),
                                MAX_MSG_LEN-REPLY_HEADER_LEN);

        //body;
        memcpy(&block[REPLY_HEADER_LEN], &reply.body[i*(MAX_MSG_LEN-REPLY_HEADER_LEN)], bodyLen);

        //printf("%s\n", &reply.body[i*(MAX_MSG_LEN-REPLY_HEADER_LEN)]);
        hashdata_t blockHash;
        get_data_sha(&block[REPLY_HEADER_LEN], blockHash, bodyLen, SHA256_HASH_SIZE);

        // 4 bytes -   Length of response body,
        //             unsigned integer in network byte-order
        memcpy(&block[0], &bodyLen, 4);
        swap_bytes(&block[0], 0, 4);
        // 4 bytes -   Status Code of the response,
        //             unsigned integer in network byte-order
        memcpy(&block[4], &reply.header.status, 4);
        swap_bytes(&block[4], 0, 4);
        // 4 bytes -   Block number, a zero-based count of which block in
        //             a potential series of replies this is,
        //             unsigned integer in network byte-order
        memcpy(&block[8], &i, 4);
        swap_bytes(&block[8], 0, 4);
        // 4 bytes -   Block count, the total number of blocks to be sent,
        //             unsigned integer in network byte-order
        memcpy(&block[12], &reply.header.block_count, 4);
        swap_bytes(&block[12], 0, 4);
        // 32 bytes -  Block hash, a hash of the response data in this
        //             message only, as UTF-8 encoded bytes
        memcpy(&block[16], &blockHash, SHA256_HASH_SIZE);
        // 32 bytes -  Total hash, a hash of the total data to be sent
        //             across all blocks, as UTF-8 encoded bytes
        memcpy(&block[48], &reply.header.total_hash, SHA256_HASH_SIZE);
        
        compsys_helper_writen(connfd, block, REPLY_HEADER_LEN+bodyLen);
    }
}
//--------------------------------------------------

/*
 * Handle any 'register' type requests, as defined in the asignment text. This
 * should always generate a response.
 */
// void handle_register(int connfd, char* client_ip, int client_port_int)
// {
void handle_register(int connfd, Address_t client_address)
{
    // Your code here. This function has been added as a guide, but feel free 
    // to add more, or work in other parts of the code
    Reply_t reply;
    reply.header.status = STATUS_PEER_EXISTS; 
    if (add_peer(client_address) == 0) {
        reply.header.status = STATUS_OK; 
        pthread_mutex_lock(&network_mutex);
        reply.header.length = 20*peer_count;
        
        char replyBody[reply.header.length];
        memset(replyBody, 0, reply.header.length);
        reply.body = replyBody;
        
        for (uint32_t i = 0; i < peer_count; i++) {
            memcpy(&reply.body[i*20], network[i]->ip, IP_LEN);
            uint32_t p = atoi(network[i]->port);
            memcpy(&reply.body[(i*20)+IP_LEN], &p, 4);
            swap_bytes(&reply.body[(i*20)+IP_LEN], 0, 4);
        }
        pthread_mutex_unlock(&network_mutex);

        handle_reply(connfd, reply);

        char informBody[20] = {'\0'};
        memcpy(&informBody[0], client_address.peer.ip, IP_LEN);
        memcpy(&informBody[IP_LEN], &client_address.port, 4);
        swap_bytes(&informBody[IP_LEN], 0, 4);

        pthread_mutex_lock(&network_mutex);
        for (u_int32_t i = 0; i < peer_count; i++) {
            if (cmp_PeerAddress(*network[i], *my_address) != 0 &&
                cmp_PeerAddress(*network[i], client_address.peer) != 0) {
                send_message(*network[i], 3, informBody, 20);
            }
        }
        pthread_mutex_unlock(&network_mutex);
    } else {
        handle_reply(connfd, reply);
    }
}

/*
 * Handle 'inform' type message as defined by the assignment text. These will 
 * never generate a response, even in the case of errors.
 */
void handle_inform(char* request)
{
    // Your code here. This function has been added as a guide, but feel free 
    // to add more, or work in other parts of the code
    Address_t address;
    memset(address.peer.ip, 0, IP_LEN);
    memset(address.peer.port, 0, PORT_LEN);
    memcpy(&address.peer.ip[0], &request[0], IP_LEN);
    swap_bytes(&request[IP_LEN], 0, 4);
    address.port = *(uint32_t*)&request[IP_LEN];
    sprintf(address.peer.port, "%u", address.port);
    add_peer(address);
}

/*
 * Handle 'retrieve' type messages as defined by the assignment text. This will
 * always generate a response
 */
// void handle_retreive(int connfd, char* request)
void handle_retreive(int connfd, char* request, uint32_t pathLen)
{
    // Your code here. This function has been added as a guide, but feel free 
    // to add more, or work in other parts of the code
    Reply_t reply;
    
    FILE *fp;
    char output_file_path[strlen(filePath)+pathLen+1];
    memset(output_file_path, 0, strlen(filePath)+pathLen+1);
    strcpy(output_file_path, filePath);
    memcpy(&output_file_path[strlen(filePath)], request, strlen(request));
    pthread_mutex_lock(&retrieving_mutex);
    fp = fopen(output_file_path,"r");
    if (fp == NULL) { 
            pthread_mutex_unlock(&retrieving_mutex);
            printf("File Not Found!\n");
            reply.header.status = STATUS_BAD_REQUEST;
            handle_reply(connfd, reply);
    } else {
        reply.header.status = STATUS_OK;
        fseek(fp, 0L, SEEK_END);
        if (ftell(fp) <= 0) {
            printf("File empty!\n");
            reply.header.length = 0;
        } else {
            reply.header.length = (u_int32_t)ftell(fp);
        }
        char fileContents[reply.header.length + 1];
        memset(fileContents, 0 , reply.header.length + 1);
        fseek(fp, 0L, SEEK_SET);
        if (reply.header.length > 0) {
            fread(fileContents, sizeof(char), reply.header.length, fp);
        }
        fclose(fp);
        pthread_mutex_unlock(&retrieving_mutex);
        reply.body = fileContents;
        handle_reply(connfd, reply);
    }
}


/*
 * Handler for all server requests. This will call the relevent function based 
 * on the parsed command code
 */
void handle_server_request(int connfd)
{
    // Your code here. This function has been added as a guide, but feel free 
    // to add more, or work in other parts of the code
    Address_t client_address;
    memset(client_address.peer.ip, 0, IP_LEN);
    memset(client_address.peer.port, 0, PORT_LEN);
    RequestHeader_t header;
    memset(header.ip, 0, IP_LEN);
    
    char h[REQUEST_HEADER_LEN + 1] = {'\0'};
    compsys_helper_readn(connfd, &h, REQUEST_HEADER_LEN);
    // 16 bytes -  The IP address the sending peer is listening on,
    //             as UTF-8 encoded bytes
    memcpy(header.ip, &h[0], IP_LEN);
    memcpy(client_address.peer.ip, header.ip, IP_LEN);
    // 4 bytes -   The port the sending peer is listening on,
    //             unsigned integer in network byte-order
    swap_bytes(&h[IP_LEN], 0, 4);
    header.port = *(uint32_t*)&h[IP_LEN];
    client_address.port = header.port;
    sprintf(client_address.peer.port, "%u", client_address.port);
    // 4 bytes -   The command code desribing the nature of this
    //             message, unsigned integer in network byte-order
    swap_bytes(&h[20], 0, 4);
    header.command = *(uint32_t*)&h[20];
    // 4 bytes -   The length of the request body, unsigned integer
    //             in network byte-order
    swap_bytes(&h[24], 0, 4);
    header.length = *(uint32_t*)&h[24];

    char body[header.length+1];
    if (header.length > 0) {
        memset(&body, 0, header.length+1);
        compsys_helper_readn(connfd, &body, header.length);
    }

    switch (header.command) {
        case 1: //Register
            handle_register(connfd, client_address);
            break;
        case 2: //Retreive
            handle_retreive(connfd, body, header.length);
            break;
        case 3: //Inform
            handle_inform(body);
            break;
        default:
            break;
    }

}

//--------------------------------------------------
void* sub_server_thread(void *vargp) {
 int connfd = *((int *)vargp);
 pthread_detach(pthread_self()); 
 free(vargp); 
 handle_server_request(connfd);
 close(connfd);
 return NULL;
}
//--------------------------------------------------

/*
 * Function to act as basis for running the server thread. This thread will be
 * run concurrently with the client thread, but is infinite in nature.
 */
void* server_thread(){
    // Your code here. This function has been added as a guide, but feel free 
    // to add more, or work in other parts of the code
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr clientaddr;
    pthread_t tid;
    listenfd = compsys_helper_open_listenfd(my_address->port);
    while (1) {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, &clientaddr, (socklen_t*)&clientlen); 
        pthread_create(&tid, NULL, sub_server_thread, connfdp);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    // Initialise with known junk values, so we can test if these were actually
    // present in the config or not
    struct PeerAddress peer_address;
    memset(peer_address.ip, '\0', IP_LEN);
    memset(peer_address.port, '\0', PORT_LEN);
    memcpy(peer_address.ip, "x", 1);
    memcpy(peer_address.port, "x", 1);

    // Users should call this script with a single argument describing what 
    // config to use
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <config file>\n", argv[0]);
        exit(EXIT_FAILURE);
    } 

    my_address = (PeerAddress_t*)malloc(sizeof(PeerAddress_t));
    memset(my_address->ip, '\0', IP_LEN);
    memset(my_address->port, '\0', PORT_LEN);

    // Read in configuration options. Should include a client_ip, client_port, 
    // server_ip, and server_port
    char buffer[128];
    fprintf(stderr, "Got config path at: %s\n", argv[1]);
    FILE* fp = fopen(argv[1], "r");
    while (fgets(buffer, 128, fp)) {
        if (starts_with(buffer, MY_IP)) {
            memcpy(&my_address->ip, &buffer[strlen(MY_IP)], 
                strcspn(buffer, "\r\n")-strlen(MY_IP));
            if (!is_valid_ip(my_address->ip)) {
                fprintf(stderr, ">> Invalid client IP: %s\n", my_address->ip);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, MY_PORT)) {
            memcpy(&my_address->port, &buffer[strlen(MY_PORT)], 
                strcspn(buffer, "\r\n")-strlen(MY_PORT));
            if (!is_valid_port(my_address->port)) {
                fprintf(stderr, ">> Invalid client port: %s\n", 
                    my_address->port);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, PEER_IP)) {
            memcpy(peer_address.ip, &buffer[strlen(PEER_IP)], 
                strcspn(buffer, "\r\n")-strlen(PEER_IP));
            if (!is_valid_ip(peer_address.ip)) {
                fprintf(stderr, ">> Invalid peer IP: %s\n", peer_address.ip);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, PEER_PORT)) {
            memcpy(peer_address.port, &buffer[strlen(PEER_PORT)], 
                strcspn(buffer, "\r\n")-strlen(PEER_PORT));
            if (!is_valid_port(peer_address.port)) {
                fprintf(stderr, ">> Invalid peer port: %s\n", 
                    peer_address.port);
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(fp);

    retrieving_files = malloc(file_count * sizeof(FilePath_t*));
    srand(time(0));

    network = malloc(sizeof(PeerAddress_t*));
    //network[0] = my_address;
    //----------------------------------------
    network[0] = malloc(sizeof(PeerAddress_t)); 
    memcpy(network[0]->ip, my_address->ip, IP_LEN);
    memcpy(network[0]->port, my_address->port, PORT_LEN);
    //----------------------------------------
    peer_count = 1;

    //----------------------------------------
    printf("MyIP: %s, MyPort: %s\n", &my_address->ip[0], my_address->port);
    printf("PeerIP: %s, PeerPort: %s\n", &peer_address.ip[0], peer_address.port);
    //----------------------------------------

    // Setup the client and server threads 
    pthread_t client_thread_id;
    pthread_t server_thread_id;
    if (peer_address.ip[0] != 'x' && peer_address.port[0] != 'x')
    {
        //----------------------------------------
        memset(filePath, 0, strlen(filePath));
        strcat(filePath, "../Data");
        mkdir(filePath);
        strcat(filePath, "/");
        strcat(filePath, my_address->port);
        mkdir(filePath);
        strcat(filePath, "/");
        //----------------------------------------   
        pthread_create(&client_thread_id, NULL, client_thread, &peer_address);
    } 
    pthread_create(&server_thread_id, NULL, server_thread, NULL);
    
    // Start the threads. Note that the client is only started if a peer is 
    // provided in the config. If none is we will assume this peer is the first
    // on the network and so cannot act as a client.
    if (peer_address.ip[0] != 'x' && peer_address.port[0] != 'x')
    {
        pthread_join(client_thread_id, NULL);
    }
    pthread_join(server_thread_id, NULL);
    
    exit(EXIT_SUCCESS);
}