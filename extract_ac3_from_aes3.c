 #include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "a52.h"

#define AC3_FRAME_SZ_MS 32
#define BUF_SIZE (1536*8)

#define SMPTE340_PA (0xf872)
#define SMPTE340_PB (0x4e1f)

#define USAGE "./a.out <input.ac3> <output.ac3> <num pkts to dump>\n"

void byte_swap(uint8_t *buf, int buf_size) {
    
    uint16_t   *out_buf = (uint16_t*)buf;
    int     i;

    for (i=0; i<buf_size/2; i++) {
        out_buf[i] = ((out_buf[i] << 8) | (out_buf[i] >> 8));
    }
}

static int dump_ac3_pkt(uint8_t *buf, int buf_size, FILE *outFile) {

    int frmsize=0;
    int i;
    static ;
    uint8_t loc_buf[BUF_SIZE];

    memcpy(loc_buf, buf, buf_size);

    if(a52_get_sync(loc_buf) != 0x770b){
        printf ("Couldn't SYNC. BYTE is 0x%x\n", a52_get_sync(loc_buf));
        return -1;
    }
        
    //byte_swap(loc_buf, buf_size);
    fwrite(loc_buf, 1, buf_size, outFile);
    
    /*frmsize =  a52_get_frame_size(a52_get_fscod(buf), a52_get_frmsizecod(buf));
    printf (" sync byte=0x%x %x framesz = %d\n", 
        buf[0], buf[1], frmsize);

    for(i=0;i<frmsize;i++) {
        if (i%8 == 0)
            printf ("\n");
        printf ("0x%02x, ", buf[i]);
    }    
    printf ("\n\n");*/

    return frmsize;

}

int get_aes3_syc_offset(uint8_t *buf, int read_bytes, int *got_sync) {
    
    int     i;

    for (i=0; i<read_bytes; i=i+2) {

        if ((*(unsigned short*)(buf+i) == SMPTE340_PA) && (*(unsigned short*)(buf+i+2) == SMPTE340_PB)) {
            
            //printf ("Found sync 0x%x at %d byte offset\n", *(unsigned short*)(buf+i), i);
            *got_sync = 1;
            return i;
        }
    }
    *got_sync = 0;
    return read_bytes;
}

int main(int argc,char **argv) {

    char    input_file[256];
    char    output_file[256];
    FILE    *inpFile;
    FILE    *outFile;
    int     num_pkts;
    uint8_t buf[BUF_SIZE];
    uint8_t *tmp_buf;

    int     read_bytes;
    int     consumed_bytes;
    int     ret;
    int     pkts_dumped=0;
    int     exit_flag=0;

    int     sync_offset=0;
    int     got_sync=0;
    int     ac3_burst_sz=0;
    int     pos;
    int     incorrect_off_after=0;

    if (argc != 4) {
        printf (USAGE);
        return 1;
    }

    strcpy (input_file, argv[1]);
    strcpy (output_file, argv[2]);
    num_pkts = atoi(argv[3]);
    
    inpFile = fopen(input_file, "rb");
    if (!inpFile) {
        printf ("Failed to open input file %s\n", input_file);
        perror("ERROR:: ");
        return 1;
    }
    
    outFile = fopen(output_file, "wb");
    if (!inpFile) {
        printf ("Failed to open output file %s\n", output_file);
        perror("ERROR:: ");
        return 1;
    }
    
    while (((read_bytes = fread(buf, 1, BUF_SIZE, inpFile)) > 0) && (!exit_flag))
    //read_bytes = fread(buf, 1, BUF_SIZE, inpFile);
    {

        sync_offset += get_aes3_syc_offset(buf, read_bytes, &got_sync);
        if (got_sync) break;
    }
    
    if (!got_sync) {
        printf ("Reached EOF without getting sync byte.\n\n");
        return 1;
    }

    /*if (sync_offset > (1536*16)) {
        
        printf ("AES3 sync not found even after %d bytes. Sync @ %d\n", (1536*16), sync_offset);
        return 1;
    }*/

    fseek(inpFile, sync_offset, SEEK_SET);
    if (ret!=0) printf ("fseek failed\n\n\n");

    while (((read_bytes = fread(buf, 1, BUF_SIZE, inpFile)) > 0) && (!exit_flag)) {
        
        consumed_bytes = 0;
        tmp_buf        = buf;
        got_sync       = 0;

        if ((*(unsigned short*)(tmp_buf) == SMPTE340_PA) && (*(unsigned short*)(tmp_buf+2) == SMPTE340_PB)) {
            
            ac3_burst_sz = (int)*(unsigned short*)(tmp_buf+6) / 8;
            //printf ("ac3_burst_sz = %d\n", ac3_burst_sz);
            ret = dump_ac3_pkt(tmp_buf+8, ac3_burst_sz, outFile);
            sync_offset = get_aes3_syc_offset((buf+4), BUF_SIZE-4, &got_sync);
            incorrect_off_after++;
            if (got_sync) {
                if (sync_offset != (1536*4-4)) {
                    printf ("Got incorrect offset after %d writes\n", incorrect_off_after);
                    incorrect_off_after = 0;
                }
                pos = ftell(inpFile);
                ret = fseek(inpFile, (pos-read_bytes+sync_offset+4), SEEK_SET);
                if (ret!=0) printf ("fseek failed\n");
            } else {
            pos = ftell(inpFile);
                printf ("Couldnt get sync in main loop buffer. Read %d KB\n", pos/1024);
            tmp_buf = tmp_buf +(1536*4) - 32;
            int i;
            for(i=0;i<200;i=i+2) {
                if (i%8 == 0)
                    printf ("\n");
                printf ("0x%02x%02x, ", tmp_buf[i+1], tmp_buf[i]);
            }
            }
        }
        else {
            pos = ftell(inpFile);
            printf ("Didn't get sync byte in main loop. Read %d KB\n", pos/1024);
            tmp_buf = tmp_buf;
            int i;
            for(i=0;i<200;i=i+2) {
                if (i%8 == 0)
                    printf ("\n");
                printf ("0x%02x%02x, ", tmp_buf[i+1], tmp_buf[i]);
            }
            return 1;
        }

    }
    return 0;
}
