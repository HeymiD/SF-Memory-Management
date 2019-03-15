/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"

void init_first_page(){

        sf_mem_grow();

        sf_prologue* prg=sf_mem_start();

        prg->header.info.allocated=1;
        prg->header.info.prev_allocated=0;
        prg->header.info.block_size=0;
        prg->header.info.requested_size=0;

        prg->footer.info.allocated=1;
        prg->footer.info.prev_allocated=0;
        prg->footer.info.block_size=0;
        prg->footer.info.requested_size=0;

        sf_epilogue* epg = sf_mem_end()-8;
        epg->footer.info.allocated=1;
        epg->footer.info.prev_allocated=1;
        epg->footer.info.block_size=0;
        epg->footer.info.requested_size=0;

        sf_header* block1_header=sf_mem_start() + 40;
        block1_header->info.allocated=0;
        block1_header->info.prev_allocated=1;
        block1_header->info.block_size=4048>>4;
        block1_header->info.requested_size=0;

        sf_footer* block1_footer=sf_mem_start()+40+(block1_header->info.block_size<<4)-8;
        block1_footer->info.allocated=0;
        block1_footer->info.prev_allocated=1;
        block1_footer->info.block_size=4048>>4;
        block1_footer->info.requested_size=0;

        sf_free_list_node* first_node  = sf_add_free_list(4048,&sf_free_list_head);

        first_node->head.links.next=block1_header;
        first_node->head.links.prev=block1_header;
        block1_header->links.next=&first_node->head;
        block1_header->links.prev=&first_node->head;

}

sf_free_list_node* find_free_list(size_t s, sf_free_list_node* current, sf_free_list_node* head){

    while(current != head){
        if(current->size >= s){
            return current;
        }
        current=current->next;
    }

    return head;
}
sf_free_list_node* find_free_list_equal(size_t s, sf_free_list_node* current, sf_free_list_node* head){

    while(current != head){
        if(current->size == s){
            return current;
        }
        current=current->next;
    }

    return head;
}
sf_free_list_node* find_free_list_smaller(size_t s, sf_free_list_node* current, sf_free_list_node* head){

    while(current != head){
        if(current->size < s){
            return current;
        }
        current=current->prev;
    }

    return head;
}
void remove_from_free_list(sf_header* block){
    (block->links.prev)->links.next = block->links.next;
    (block->links.next)->links.prev = block->links.prev;
}

void add_to_free_list(sf_free_list_node* node, sf_header* new_block){
    new_block->links.next=(node->head.links.next);
    node->head.links.next->links.prev=new_block;
    node->head.links.next=new_block;
    new_block->links.prev=&(node->head);
}
void fill_prev_alloc(sf_header* pointer){
    sf_footer* footer_p = (void*)pointer-8;
        if(footer_p->info.allocated==0){
            pointer->info.prev_allocated=0;
        }
        else{
            pointer->info.prev_allocated=1;
        }
}



void* coalesce(void* block){
    sf_header* block_p = block;
    sf_footer* footer_prev_p = (void*)block_p-8;
    sf_header* header_next_p = (void*)block_p+(block_p->info.block_size<<4);
    //header_next_p+=(block_p->info.block_size<<4);
    sf_header* header_prev_p = (void*)block_p - (footer_prev_p->info.block_size<<4);
    size_t s = block_p->info.block_size<<4;
    //printf("Current : %lu\n", s);
    //printf("Prev: %d\n", footer_prev_p->info.block_size<<4);


    unsigned prev_al = block_p->info.prev_allocated;
    unsigned next_al = (header_next_p)->info.allocated;
        if((prev_al!=0) && (next_al!=0)){
            return (void*)block_p;
        }
        else if(prev_al!=0 && next_al==0){

            s=(header_next_p->info.block_size<<4)+s;

            //printf("%lu, 1\n", s);

            remove_from_free_list(block_p);
            remove_from_free_list(header_next_p);
            block_p->info.block_size=s>>4;
            //block_p->info.prev_allocated=1;
            sf_free_list_node* node_to_add_to = find_free_list_equal(s,sf_free_list_head.next,&sf_free_list_head);

            if(node_to_add_to==&sf_free_list_head){
                sf_free_list_node* new_node = sf_add_free_list(s,find_free_list_smaller(s,sf_free_list_head.prev,&sf_free_list_head)->next);
                add_to_free_list(new_node,block_p);
            }
            else{
                add_to_free_list(node_to_add_to,block_p);
            }

            sf_header* next_block_after_c = (void*)header_next_p+(header_next_p->info.block_size<<4);
            if((void*)next_block_after_c<sf_mem_end()-8){next_block_after_c->info.prev_allocated=0;}
            sf_footer* new_footer = (void*)header_next_p+(header_next_p->info.block_size<<4)-8;
            //new_footer+=block_p->info.block_size<<4;
            new_footer->info.block_size=s>>4;
            new_footer->info.prev_allocated=prev_al;
            new_footer->info.allocated=block_p->info.allocated;
            new_footer->info.requested_size=block_p->info.requested_size;
        }

        else if(prev_al==0 && next_al!=0){

            s+=header_prev_p->info.block_size<<4;
            //printf("%lu, 2\n", s);
            remove_from_free_list(block_p);
            remove_from_free_list(header_prev_p);

            block_p=header_prev_p;
            block_p->info.block_size=s>>4;
            //block_p->info.prev_allocated=1;
            block_p->info.prev_allocated=header_prev_p->info.prev_allocated;
            block_p->info.allocated=0;
            block_p->info.requested_size=0;


            sf_header* next_block_after_c = (void*)block_p+(block_p->info.block_size<<4);
            if((void*)next_block_after_c<sf_mem_end()-8){next_block_after_c->info.prev_allocated=0;}

            sf_free_list_node* node_to_add_to = find_free_list_equal(s,sf_free_list_head.next,&sf_free_list_head);

            if(node_to_add_to==&sf_free_list_head){
                sf_free_list_node* new_node = sf_add_free_list(s,find_free_list_smaller(s,sf_free_list_head.prev,&sf_free_list_head)->next);
                add_to_free_list(new_node,block_p);
            }
            else{
                add_to_free_list(node_to_add_to,block_p);
            }

            sf_footer* new_footer = (void*)block_p+(block_p->info.block_size<<4)-8;
            //printf("%lu\n", s);
            //new_footer+=block_p->info.block_size<<4;
            new_footer->info.block_size=s>>4;
            new_footer->info.prev_allocated=block_p->info.prev_allocated;
            new_footer->info.allocated=0;
            new_footer->info.requested_size=block_p->info.requested_size;
            //block_p=header_prev_p;
        }

        else{
            s+=(header_prev_p->info.block_size<<4)+(header_next_p->info.block_size<<4);
            //printf("%lu, 3\n", s);
            remove_from_free_list(block_p);
            remove_from_free_list(header_prev_p);
            remove_from_free_list(header_next_p);
            //if((void*)header_next_p<(sf_mem_end()-8)){
              //  remove_from_free_list(header_next_p);
            //}

            //remove_from_free_list(header_next_p);
            block_p=header_prev_p;
            block_p->info.block_size=s>>4;
            block_p->info.prev_allocated=header_prev_p->info.prev_allocated;

            sf_header* next_block_after_c = (void*)header_prev_p+(block_p->info.block_size<<4);
            if((void*)next_block_after_c<sf_mem_end()-8){next_block_after_c->info.prev_allocated=0;}

            sf_free_list_node* node_to_add_to = find_free_list_equal(s,sf_free_list_head.next,&sf_free_list_head);

            if(node_to_add_to==&sf_free_list_head){
                sf_free_list_node* new_node = sf_add_free_list(s,find_free_list_smaller(s,sf_free_list_head.prev,&sf_free_list_head)->next);
                add_to_free_list(new_node,block_p);
            }
            else{
                add_to_free_list(node_to_add_to,block_p);
            }

            //block_p=header_prev_p;

            sf_footer* new_footer = (void*)header_prev_p+(block_p->info.block_size<<4)-8;
            //new_footer+=block_p->info.block_size<<4;
            new_footer->info.block_size=s>>4;
            new_footer->info.prev_allocated=block_p->info.prev_allocated;
            new_footer->info.allocated=block_p->info.allocated;
            new_footer->info.requested_size=block_p->info.requested_size;

            //block_p=header_prev_p;


        }
        return (void*)block_p;
}

unsigned int add_pages(size_t rqs_size){

    sf_header* pointer = sf_mem_end()-8;
    sf_footer* foot_p = sf_mem_end() - 16;
    //int added = 0;
    //size_t new_size = foot_p->info.block_size<<4;
    //printf("%lu\n", new_size);
    /*pointer=pointer-8;
    pointer->info.allocated=0;
    pointer->info.block_size=(number_pages*4096)>>4;
    fill_prev_alloc(pointer);*/

    size_t new_size=foot_p->info.block_size<<4;
    while(new_size<rqs_size){
        if(sf_mem_grow()==NULL){

            sf_epilogue* new_epg = sf_mem_end()-8;
    new_epg->footer.info.allocated=1;
    new_epg->footer.info.prev_allocated=1;
    new_epg->footer.info.block_size=0;
    new_epg->footer.info.requested_size=0;

    //new_size=new_size-(foot_p->info.block_size<<4)+8;

    //printf("%lu\n", new_size);

    pointer->info.allocated=0;
    pointer->info.block_size=(new_size-(foot_p->info.block_size<<4))>>4;
    pointer->info.requested_size=rqs_size;
    //pointer->info.prev_allocated=foot_p->info.allocated;
    fill_prev_alloc(pointer);

    //printf("Pointer : %d\n", pointer->info.prev_allocated);
    //printf("%p\n",pointer);

    sf_free_list_node* new_free_list = sf_add_free_list(pointer->info.block_size<<4,&sf_free_list_head);
    add_to_free_list(new_free_list,pointer);

    sf_footer* f_pointer = (void*)new_epg-8;//(void*)pointer + (pointer->info.block_size<<4) - 8;
    f_pointer->info.allocated=0;
    f_pointer->info.block_size = new_size>>4;
    f_pointer->info.prev_allocated=pointer->info.prev_allocated;

    //coalesce((void*)pointer);



    coalesce((void*)pointer);


            return 0;
        }
        else{new_size=new_size+4096;}
    }

    sf_epilogue* new_epg = sf_mem_end()-8;
    new_epg->footer.info.allocated=1;
    new_epg->footer.info.prev_allocated=1;
    new_epg->footer.info.block_size=0;
    new_epg->footer.info.requested_size=0;

    //new_size=new_size-(foot_p->info.block_size<<4)+8;

    //printf("%lu\n", new_size);

    pointer->info.allocated=0;
    pointer->info.block_size=(new_size-(foot_p->info.block_size<<4))>>4;
    pointer->info.requested_size=rqs_size;
    fill_prev_alloc(pointer);

    //printf("%d\n", pointer->info.prev_allocated);

    sf_free_list_node* new_free_list = sf_add_free_list(pointer->info.block_size<<4,&sf_free_list_head);
    add_to_free_list(new_free_list,pointer);

    sf_footer* f_pointer = (void*)new_epg-8;//(void*)pointer + (pointer->info.block_size<<4) - 8;
    f_pointer->info.allocated=0;
    f_pointer->info.block_size = new_size>>4;
    f_pointer->info.prev_allocated=pointer->info.prev_allocated;

    //coalesce((void*)pointer);



    coalesce((void*)pointer);



    return 1;


}





void split(sf_header* current, size_t allocated){//,sf_free_list_node* head){
    size_t new_size = (current->info.block_size<<4) - allocated;

    //printf("allocated: %lu\n", new_size);
    //printf("block Size: %d\n", current->info.block_size<<4);
    if(new_size<32){
        return;
    }
    else{
        //current+=allocated;
        //printf("%lu\n", sf_mem_end()-(void*)current);
        remove_from_free_list(current);

        current->info.block_size=allocated>>4;
        //current->info.requested_size=allocated>>4;
        //current->info.allocated=1;

        sf_header* new_head=(void*)current+allocated;
        new_head->info.allocated=0;
        new_head->info.prev_allocated=1;
        new_head->info.requested_size=0;
        new_head->info.block_size=new_size>>4;

        sf_free_list_node* found_node = find_free_list_equal(new_head->info.block_size<<4,sf_free_list_head.next,&sf_free_list_head);
        if(found_node==&sf_free_list_head){
            sf_free_list_node* node_to_add_after_of = find_free_list_smaller(new_head->info.block_size<<4,sf_free_list_head.prev,&sf_free_list_head);
            sf_free_list_node* added_node = sf_add_free_list(new_head->info.block_size<<4,node_to_add_after_of->next);
            add_to_free_list(added_node,new_head);
            sf_footer* new_footer=(void*)new_head+(new_head->info.block_size<<4)-8;
            new_footer->info.allocated=0;
            new_footer->info.prev_allocated=1;
            new_footer->info.requested_size=0;
            new_footer->info.block_size=new_size>>4;
        }
        else{
            add_to_free_list(found_node,new_head);
            sf_footer* new_footer=(void*)new_head+(new_head->info.block_size<<4)-8;
            new_footer->info.allocated=0;
            new_footer->info.prev_allocated=1;
            new_footer->info.requested_size=0;
            new_footer->info.block_size=new_size>>4;

        }

    }

}



void *sf_malloc(size_t size) {

    size+=8;

    if(size==0){
        return NULL;
    }

    if(sf_free_list_head.next==sf_free_list_head.prev){

        init_first_page();
        //sf_show_heap();

    }

    if ((size%16)!=0){
        size = size + (16 - (size%16));
    }
    //printf("Size: %lu\n", size);
    if(size==16){
        size=size+16;
    }

    sf_free_list_node* head_p = &sf_free_list_head;

    sf_free_list_node* node_to_allocate_from_p = find_free_list(size,head_p->next,head_p);

    if(node_to_allocate_from_p==head_p){


        //sf_header* begin_new_pages = sf_mem_end()-8;
        if(add_pages(size)==0){
            sf_errno = ENOMEM;
            return NULL;
        }

        //printf("adding pages...\n");
        //coalesce(begin_new_pages);
        //sf_malloc(size);
        sf_free_list_node* node_to_allocate_from_p = find_free_list(size,head_p->next,head_p);

        sf_header* allocated_p = node_to_allocate_from_p->head.links.next;
        //allocated_p->info.block_size=size>>4;
        allocated_p->info.requested_size=size;
        allocated_p->info.allocated=1;

        sf_header* next_after_allocated = (void*)allocated_p+size;
        next_after_allocated->info.prev_allocated=1;

        /*sf_footer* footer_p = (void*)allocated_p-8;
        if(footer_p->info.allocated==0){
            allocated_p->info.prev_allocated=0;
        }
        else{
            allocated_p->info.prev_allocated=1;
        }*/
        remove_from_free_list(allocated_p);

        //printf("Block Size : %d\n", allocated_p->info.requested_size);

        split(allocated_p,size);

        //printf("Alloc_bit : %d\n", allocated_p->info.allocated);

        return (void*)allocated_p+8;


    }
    else{
        sf_header* allocated_p = node_to_allocate_from_p->head.links.next;
        //allocated_p->info.block_size=size>>4;
        allocated_p->info.requested_size=size;
        allocated_p->info.allocated=1;
        sf_header* next_after_allocated = (void*)allocated_p+size;
        next_after_allocated->info.prev_allocated=1;


        /*sf_footer* footer_p = (void*)allocated_p-8;
        if(footer_p->info.allocated==0){
            allocated_p->info.prev_allocated=0;
        }
        else{
            allocated_p->info.prev_allocated=1;
        }*/
        remove_from_free_list(allocated_p);

        //printf("Block Size : %d\n", allocated_p->info.requested_size);

        split(allocated_p,size);

        //printf("Alloc_bit : %d\n", allocated_p->info.allocated);

        return (void*)allocated_p+8;

    }






    //int a = 0;
    //void* p =0;
    //return p;
}

void sf_free(void *pp) {

    //pp=(void*)pp-8;

    if(pp==NULL){
        //printf("Pointer was null\n");
        abort();
    }

    pp=(void*)pp-8;

    if((pp<sf_mem_start()+40) || (pp>(sf_mem_end()-8))){
        //printf("Pointer points after epilogue, or before the prologue.\n");
        abort();
    }

    sf_header* new_header = pp;
    //printf("allocated: %d\n", new_header->info.allocated);

    if(new_header->info.allocated==0){
        //printf("Nothing is allocated.\n");
        abort();
    }




    size_t size = new_header->info.block_size<<4;

    new_header->info.allocated=0;
    new_header->info.requested_size=0;
    //fill_prev_alloc(new_header);
    //new_header->info.block_size=size>>4;

    sf_header* next_block = (void*)new_header + size;//(new_header->info.block_size<<4);
    next_block->info.prev_allocated=0;

    sf_free_list_node* found_node = find_free_list_equal(new_header->info.block_size<<4,sf_free_list_head.next,&sf_free_list_head);
    if(found_node==&sf_free_list_head){
            sf_free_list_node* node_to_add_after_of = find_free_list_smaller(new_header->info.block_size<<4,sf_free_list_head.prev,&sf_free_list_head);
            sf_free_list_node* added_node = sf_add_free_list(new_header->info.block_size<<4,node_to_add_after_of->next);
            add_to_free_list(added_node,new_header);
            /*sf_footer* new_footer=(void*)new_header+(new_header->info.block_size<<4)-8;
            new_footer->info.allocated=0;
            new_footer->info.prev_allocated=1;
            new_footer->info.requested_size=0;
            new_footer->info.block_size=size>>4;*/
        }
        else{
            add_to_free_list(found_node,new_header);
            /*sf_footer* new_footer=(void*)new_header+(new_header->info.block_size<<4)-8;
            new_footer->info.allocated=0;
            new_footer->info.prev_allocated=1;
            new_footer->info.requested_size=0;
            new_footer->info.block_size=size>>4;*/

        }

    sf_footer* footer = pp+size-8;
    footer->info.requested_size=new_header->info.requested_size;
    footer->info.block_size=size>>4;
    footer->info.prev_allocated=new_header->info.prev_allocated;
    footer->info.allocated=new_header->info.allocated;
    //printf("Footer block size: %d\n", footer->info.block_size<<4);




    coalesce(pp);

    return;
}

void *sf_realloc(void *pp, size_t rsize) {

    //printf("Rsize: %ld\n", rsize);
    size_t old_rsize = rsize;


    if(pp==NULL){
        //printf("It was null\n");
        sf_errno=EINVAL;
        abort();
    }

    pp=(void*)pp-8;


    if((pp<sf_mem_start()+40) || (pp>sf_mem_end()-8)){
        //printf("It was epg prg shit\n");
        abort();
    }

    sf_header* new_header = pp;

    if(new_header->info.allocated==0){
        //printf("It was not allocated\n");
        abort();
    }

    if(rsize==0){
        return NULL;
    }

    if(rsize==(new_header->info.block_size<<4)){
        return (void*)pp+8;
    }


    if((new_header->info.block_size<<4)<rsize){
        void* new_place = sf_malloc(rsize);
        memcpy(new_place,pp,new_header->info.block_size<<4);
        sf_free((void*)new_header+8);
        return (void*)new_place;

    }
    else{

        if(rsize<32){
        //new_header->info.requested_size=old_rsize;
        //return (void*)pp+8;
            rsize=32;
        }

        {
            if(((new_header->info.block_size<<4)-rsize)<32){

                new_header->info.requested_size=old_rsize;
                return (void*)pp+8;
            }

            size_t new_block_size = (new_header->info.block_size<<4)-rsize;

            new_header->info.requested_size=old_rsize;
            new_header->info.block_size=rsize>>4;
            new_header->info.allocated=1;


            //printf("New Block Size : %ld\n", new_block_size);

            sf_header* new_block_header = pp +rsize;
            new_block_header->info.block_size = new_block_size>>4;

            //printf("New Block Size : %d\n", new_block_header->info.block_size<<4);

            new_block_header->info.prev_allocated=1;
            new_block_header->info.allocated=0;

            sf_header* next_new_block = (void*)new_block_header+(new_block_size);
            next_new_block->info.prev_allocated=0;

            sf_free_list_node* found_node = find_free_list_equal(new_block_header->info.block_size<<4,sf_free_list_head.next,&sf_free_list_head);
        if(found_node==&sf_free_list_head){
            sf_free_list_node* node_to_add_after_of = find_free_list_smaller(new_block_header->info.block_size<<4,sf_free_list_head.prev,&sf_free_list_head);
            sf_free_list_node* added_node = sf_add_free_list(new_block_header->info.block_size<<4,node_to_add_after_of->next);
            add_to_free_list(added_node,new_block_header);}

        else{
            add_to_free_list(found_node,new_block_header);}

            //printf("New Block Size : %d\n", new_block_header->info.block_size<<4);

            sf_footer* new_block_footer = (void*)new_block_header + (new_block_header->info.block_size<<4) - 8;
            new_block_footer->info.block_size=new_block_header->info.block_size;
            new_block_footer->info.prev_allocated=1;
            new_block_footer->info.allocated=0;

            coalesce((void*)new_block_header);


            return (void*)new_header+8;

        }



    }
    //return NULL;


}

//void add_free_list(sf_free_list_node new_list){}

