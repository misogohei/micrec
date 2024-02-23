#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <lame/lame.h>

int main(int argc, char *argv[])
{
#define ERR_CHK(m, e)                                \
    do                                               \
    {                                                \
        if ((e) < 0)                                 \
        {                                            \
            printf("%s:%s\n", (m), snd_strerror(e)); \
            goto cleanup;                            \
        }                                            \
    } while (0)

#define ERR_LAME(m, e)                 \
    do                                 \
    {                                  \
        if ((e) < 0)                   \
        {                              \
            printf("%s [%d]\n", m, e); \
            goto cleanup;              \
        }                              \
    } while (0)

    int err;
    int rec_sec;
    char *mic_device;
    snd_pcm_t *p_dev = NULL;
    snd_pcm_hw_params_t *p_hw_param = NULL;
    snd_pcm_format_t in_format = SND_PCM_FORMAT_S16_LE;
    int num_channel = 1;
    int num_frame;
    uint8_t *mic_buf = NULL;
    int mic_buf_size;
    unsigned int sampling_rate = 44100;
    unsigned char *mp3_buf = NULL;
    int mp3_output_size;
    int mp3_buf_size;
    int mp3_num_samples;
    FILE *fp = NULL;

    if (argc != 3)
    {
        printf("micrec devicename record_sec\n");
        return EXIT_FAILURE;
    }
    rec_sec = atoi(argv[2]);
    if (rec_sec <= 0)
    {
        printf("record time should be >= 1.\n");
        return EXIT_FAILURE;
    }
    mic_device = argv[1];

    lame_global_flags *p_lame = lame_init();
    if (p_lame == NULL)
    {
        goto cleanup;
    }

    err = snd_pcm_open(&p_dev, mic_device, SND_PCM_STREAM_CAPTURE, 0);
    ERR_CHK("snd_pcm_open", err);

    err = snd_pcm_hw_params_malloc(&p_hw_param);
    ERR_CHK("snd_pcm_hw_params_malloc", err);

    err = snd_pcm_hw_params_any(p_dev, p_hw_param);
    ERR_CHK("snd_pcm_hw_params_any", err);

    err = snd_pcm_hw_params_set_access(p_dev,
                                       p_hw_param,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    ERR_CHK("snd_pcm_hw_params_set_access", err);

    err = snd_pcm_hw_params_set_format(p_dev, p_hw_param, in_format);
    ERR_CHK("snd_pcm_hw_params_set_format", err);

    // calculate nearest and valid sampling rate.
    err = snd_pcm_hw_params_set_rate_near(p_dev,
                                          p_hw_param,
                                          &sampling_rate,
                                          NULL);
    ERR_CHK("snd_pcm_hw_params_set_rate_near", err);
    // number of frames for 1 second.
    num_frame = sampling_rate;
    printf("Sampling Rate = %d\n", sampling_rate);

    err = snd_pcm_hw_params_set_channels(p_dev,
                                         p_hw_param,
                                         num_channel);
    ERR_CHK("snd_pcm_hw_params_set_channels", err);

    // set all param to device.
    err = snd_pcm_hw_params(p_dev, p_hw_param);
    ERR_CHK("snd_pcm_hw_params", err);

    // calc buffer size and allocate.
    mic_buf_size = num_frame * snd_pcm_format_width(in_format) / 8 * num_channel;
    mic_buf = malloc(mic_buf_size * rec_sec);

    // recording
    err = snd_pcm_readi(p_dev, mic_buf, num_frame * rec_sec);
    ERR_CHK("snd_pcm_readi", err);

    // lame part
    // set encoding parameters
    err = lame_set_in_samplerate(p_lame, sampling_rate);
    ERR_LAME("lame_set_in_samplerate", err);
    err = lame_set_num_channels(p_lame, num_channel);
    ERR_LAME("lame_set_num_channels", err);
    err = lame_set_mode(p_lame, MONO); // see lame.h: MPEG_mode_e
    ERR_LAME("lame_set_mode", err);
    err = lame_init_params(p_lame);
    ERR_LAME("lame_init_params", err);

    // prepare buffer for encoding.
    mp3_num_samples = mic_buf_size * rec_sec / 2;
    mp3_buf_size = 1.25 * mp3_num_samples + 7200;
    mp3_buf = malloc(mp3_buf_size);
    // encode data.
    err = lame_encode_buffer(p_lame,
                             (int16_t *)mic_buf,
                             (int16_t *)mic_buf,
                             mp3_num_samples, mp3_buf, mp3_buf_size);
    ERR_LAME("encode", err);
    // err > 0 is encoded size.
    mp3_output_size = err;
    err = lame_encode_flush(p_lame, mp3_buf, mp3_buf_size);
    ERR_LAME("encode_flush", err);
    mp3_output_size += err;
    printf("encoded data size = %d\n", mp3_output_size);

    // open file to output encoded data.
    fp = fopen("out.mp3", "w+b");
    if (fp == NULL)
    {
        perror("fopen");
        goto cleanup;
    }
    if (fwrite(mp3_buf, mp3_output_size, 1, fp) <= 0)
    {
        perror("fwrite");
        goto cleanup;
    }
    // set tag.
    lame_mp3_tags_fid(p_lame, fp);

cleanup:
    if (mp3_buf != NULL)
    {
        free(mp3_buf);
    }
    if (fp != NULL)
    {
        fclose(fp);
    }
    if (p_lame != NULL)
    {
        lame_close(p_lame);
    }
    if (mic_buf != NULL)
    {
        free(mic_buf);
    }
    if (p_hw_param != NULL)
    {
        snd_pcm_hw_params_free(p_hw_param);
    }
    if (p_dev != NULL)
    {
        snd_pcm_close(p_dev);
    }

    return EXIT_SUCCESS;
}
