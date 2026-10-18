#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include "../Task.h"
#include "../Sdcard.h"
#include "../VS1053.h"
#include "../Common.h"
#include "../Display.h"
#include "../ui/elements/IconButton.h"
#include <ctype.h>
#include <string.h>

namespace nsVideoPlayer
{
    class VideoPlayer : public Task
    {
    private:
        class StartForm : public Form
        {
        public:
            bool onKey(const KBState &kb) override
            {
                if (kb.pressedKey == keyLeft)
                {
                    focusPrev();
                    return true;
                }
                if (kb.pressedKey == keyRight)
                {
                    focusNext();
                    return true;
                }
                if (kb.pressedKey == keyEsc)
                {
                    requestCloseOwnerTask();
                    return true;
                }
                return Form::onKey(kb);
            }
        };

        inline static const char caption[] PROGMEM = "Video";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xf0, 0xfc, 0xff, 0xf3, 0xfe, 0xff, 0xf7, 0x06, 0x00, 0xf6,
            0x06, 0x00, 0xf6, 0x06, 0x02, 0xf6, 0x06, 0x06, 0xf6, 0x06, 0x0e, 0xf6,
            0x06, 0x06, 0xf6, 0x06, 0x02, 0xf6, 0x06, 0x00, 0xf6, 0x06, 0x00, 0xf6,
            0x06, 0x00, 0xf6, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf3, 0x00, 0x00, 0xf0,
            0x80, 0x03, 0xf0, 0xfc, 0xfe, 0xf3, 0x80, 0x03, 0xf0, 0x00, 0x00, 0xf0};
        inline static const uint8_t openIcon[] PROGMEM = {
            0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x78, 0x00, 0xFC, 0xFC, 0x00, 0xFC,
            0xFC, 0xFF, 0xFC, 0xFC, 0xFF, 0xFD, 0xFC, 0xFF, 0xFD, 0x1C, 0x00, 0xFC,
            0x0C, 0x00, 0xFC, 0xE4, 0xFF, 0xFF, 0xF4, 0xFF, 0xFF, 0xF0, 0xFF, 0xFD,
            0xF8, 0xFF, 0xFD, 0xFC, 0xFF, 0xFC, 0xFC, 0xFF, 0xFC, 0xFC, 0x7F, 0xFC,
            0xFC, 0x3F, 0xFC, 0x00, 0x00, 0xFC};
        inline static const uint8_t exitIcon[] PROGMEM = {
            0xF8, 0xFF, 0xFD, 0xF8, 0xFF, 0xFD, 0x18, 0x80, 0xFD, 0x18, 0x80, 0xFD,
            0x18, 0x80, 0xFD, 0x18, 0x80, 0xFD, 0x18, 0x80, 0xFD, 0x80, 0x80, 0xFD,
            0x80, 0x81, 0xFD, 0xFE, 0x83, 0xFD, 0x80, 0x81, 0xFD, 0x80, 0x80, 0xFD,
            0x18, 0x80, 0xFD, 0x18, 0x80, 0xFD, 0x18, 0x80, 0xFD, 0x18, 0x80, 0xFD,
            0xF8, 0xFF, 0xFD, 0xF8, 0xFF, 0xFD};

        FsFile videoData;
        FsFile audioData;
        uint8_t audioHeader[4];
        uint16_t fpsX100 = 0;
        uint32_t videoDataSize = 0;
        uint32_t frameDelayUs = 0;
        uint32_t nextFrameTime = 0;
        uint32_t currentFrame = 0;
        uint32_t audioStart = 0;
        Form *startForm = nullptr;
        IconButton *openButton = nullptr;
        IconButton *exitButton = nullptr;
        bool pendingVideoOpen = false;
        Overlay *volumeOverlay = nullptr;
        Overlay *seekOverlay = nullptr;
        uint32_t volumeOverlayUntil = 0;
        uint32_t seekOverlayUntil = 0;
        uint32_t nextSeekStepTime = 0;
        uint64_t seekTimeUs = 0;
        bool playing = false;
        bool foreground = true;
        bool paused = false;
        bool seeking = false;
        bool spaceWasPressed = false;
        bool audioAcquired = false;
        int8_t seekDirection = 0;

        static const uint32_t overlayDurationMs = 2000;
        static const uint32_t seekStepUs = 10000000UL;
        static const uint16_t heldSeekStepMs = 150;
        static const uint32_t audioBytesPerSecond = 12000UL;

        uint16_t getStackSize() const override { return 350; }

        bool readHeader()
        {
            uint8_t flags;
            if (videoData.read((uint8_t *)&fpsX100, sizeof(fpsX100)) != sizeof(fpsX100) ||
                videoData.read(&flags, sizeof(flags)) != sizeof(flags) ||
                videoData.read((uint8_t *)&videoDataSize, sizeof(videoDataSize)) != sizeof(videoDataSize) ||
                fpsX100 == 0)
                return false;

            frameDelayUs = 100000000UL / fpsX100;
            audioStart = 7 + videoDataSize;
            return audioData.seek(audioStart);
        }

        bool openPlaybackFile(const char* path)
        {
            if (!path || !path[0])
                return false;
            
            FsFile file = sdCard.open(path, O_RDONLY);
            if (!file || file.isDirectory())
                return false;
            
            videoData.move(&file);
            return initializeOpenedPlayback();
        }

        bool initializeOpenedPlayback()
        {
            audioData.close();
            audioData.copy(&videoData);
            if (!videoData || !audioData || !readHeader())
                return false;

            currentFrame = 0;
            nextFrameTime = micros();
            paused = false;
            if (audioAcquired)
                vs1053.setMuted(false);
            playing = true;
            return true;
        }

        void closePlaybackFile()
        {
            if (videoData)
                videoData.close();
            if (audioData)
                audioData.close();
            playing = false;
        }

        uint32_t totalFrames() const { return videoDataSize / 1024UL; }
        uint64_t totalDurationUs() const { return (uint64_t)totalFrames() * frameDelayUs; }
        uint64_t currentTimeUs() const { return (uint64_t)currentFrame * frameDelayUs; }

        void removeVolumeOverlay()
        {
            if (volumeOverlay)
            {
                lcd.removeOverlay(volumeOverlay);
                volumeOverlay = nullptr;
            }
        }

        void removeSeekOverlay()
        {
            if (seekOverlay)
            {
                lcd.removeOverlay(seekOverlay);
                seekOverlay = nullptr;
            }
        }

        void showVolumeOverlay()
        {
            removeVolumeOverlay();
            volumeOverlay = lcd.addOverlay(112, 4, 8, 32);
            if (!volumeOverlay)
                return;

            lcd.drawRect(112, 4, 119, 35, modeClear, true);
            lcd.drawRect(112, 4, 119, 35);
            uint8_t fillHeight = (uint16_t)vs1053.getVolume() * 28 / 15;
            if (fillHeight)
                lcd.drawRect(114, 34 - fillHeight, 117, 34, modeSet, true);
            lcd.snapshotOverlay(volumeOverlay);
            volumeOverlayUntil = millis() + overlayDurationMs;
        }

        void showSeekOverlay()
        {
            if (!seekOverlay)
                seekOverlay = lcd.addOverlay(8, 56, 112, 8);
            if (!seekOverlay)
                return;

            lcd.drawRect(8, 56, 119, 63, modeClear, true);
            lcd.drawRect(8, 56, 119, 63);
            uint64_t duration = totalDurationUs();
            uint8_t fillWidth = duration ? seekTimeUs * 109 / duration : 0;
            if (fillWidth)
                lcd.drawRect(9, 58, 8 + fillWidth, 61, modeSet, true);
            lcd.snapshotOverlay(seekOverlay);
            seekOverlayUntil = millis() + overlayDurationMs;
        }

        void updateOverlays()
        {
            uint32_t now = millis();
            if (volumeOverlay && (int32_t)(now - volumeOverlayUntil) >= 0)
                removeVolumeOverlay();
            if (seekOverlay && (int32_t)(now - seekOverlayUntil) >= 0)
                removeSeekOverlay();
        }

        void closeStartForm()
        {
            if (startForm)
            {
                ui.closeForm(startForm);
                startForm = nullptr;
                openButton = nullptr;
                exitButton = nullptr;
            }
        }

        bool startPlayback(const char* path)
        {
            if (!path || !path[0])
                return false;

            SchedulerSuspender lock;
            closePlaybackFile();
            if (audioAcquired)
                vs1053.clearBuffer();
            return openPlaybackFile(path);
        }

        bool isAudioFrameHeader(const uint8_t *header) const
        {
            return header[0] == 0xFF && (header[1] & 0xFE) == 0xF2 &&
                   (header[2] & 0xF0) == 0xA0 && (header[2] & 0x0C) == 0 &&
                   (header[3] & 0xC0) == 0xC0;
        }

        bool seekAudioTo(uint64_t timeUs)
        {
            if (audioData.fileSize() < audioStart)
            {
                DEBUG_PRINTLN_F("VP audio: invalid start");
                return false;
            }
            uint32_t audioSize = audioData.fileSize() - audioStart;
            uint32_t target = timeUs * audioBytesPerSecond / 1000000UL;
            if (target > audioSize)
                target = audioSize;

            // Расширяем диапазон поиска: начинаем чуть раньше целевой позиции
            // чтобы найти ближайший фрейм, который не позже нужной позиции
            uint32_t searchStart = (target > 600) ? (audioStart + target - 600) : audioStart;
            uint32_t position = searchStart;
            uint32_t searchEnd = audioStart + target + 1024;
            uint32_t fileEnd = audioStart + audioSize;
            if (searchEnd > fileEnd)
                searchEnd = fileEnd;

            uint32_t bestPosition = 0;
            int32_t bestDistance = INT32_MAX;

            // Ищем ближайший MP3-фрейм к целевой позиции
            for (; position + 4 <= searchEnd; position++)
            {
                size_t read;
                {
                    SchedulerSuspender lock;
                    if (!audioData.seek(position))
                    {
                        DEBUG_PRINT_F("VP audio seek fail at ");
                        DEBUG_PRINTLN(position);
                        return false;
                    }
                    read = audioData.read(audioHeader, sizeof(audioHeader));
                    if (read == sizeof(audioHeader) && isAudioFrameHeader(audioHeader))
                    {
                        // Нашли фрейм. Проверяем, насколько он близок к цели
                        int32_t distance = (int32_t)position - (int32_t)(audioStart + target);

                        // Предпочитаем фрейм, который ближе к цели и не позже её
                        if (distance <= 0 && -distance < bestDistance)
                        {
                            bestPosition = position;
                            bestDistance = -distance;
                        }
                        else if (bestDistance == INT32_MAX && distance > 0 && distance < 576)
                        {
                            // Если не нашли фрейм до цели, берём первый после, но не дальше одного фрейма
                            bestPosition = position;
                            bestDistance = distance;
                        }
                    }
                }
            }

            if (bestPosition > 0)
            {
                SchedulerSuspender lock;
                return audioData.seek(bestPosition);
            }

            DEBUG_PRINT_F("VP audio sync not found, target=");
            DEBUG_PRINT(target);
            DEBUG_PRINT_F(" range=");
            DEBUG_PRINT(searchStart);
            DEBUG_PRINT_F("..");
            DEBUG_PRINTLN(searchEnd);
            return false;
        }

        void seekVideoTo(uint64_t timeUs)
        {
            seekTimeUs = timeUs;
            currentFrame = seekTimeUs / frameDelayUs;
            if (currentFrame >= totalFrames() && totalFrames())
                currentFrame = totalFrames() - 1;
            if (foreground)
            {
                if (!renderFrame())
                {
                    DEBUG_PRINTLN_F("VP frame error while seeking");
                    currentFrame = totalFrames() ? totalFrames() - 1 : 0;
                }
            }
            showSeekOverlay();
        }

        void beginSeek(int8_t direction)
        {
            seeking = true;
            seekDirection = direction;
            seekTimeUs = currentTimeUs();
            vs1053.clearBuffer();
            nextSeekStepTime = 0;
        }

        void updateSeek()
        {
            uint32_t now = millis();
            if ((int32_t)(now - nextSeekStepTime) < 0)
                return;

            int64_t target = (int64_t)seekTimeUs + (int64_t)seekDirection * seekStepUs;
            if (target < 0)
                target = 0;
            uint64_t lastFrameTime = totalFrames() > 0 ? (uint64_t)(totalFrames() - 1) * frameDelayUs : 0;
            if ((uint64_t)target > lastFrameTime)
                target = lastFrameTime;

            seekVideoTo((uint64_t)target);
            nextSeekStepTime = now + heldSeekStepMs;
        }

        void finishSeek()
        {
            seeking = false;
            seekDirection = 0;
            if (!seekAudioTo(seekTimeUs))
            {
                DEBUG_PRINTLN_F("VP resume without audio after seek");
                vs1053.clearBuffer();
            }
            currentFrame = seekTimeUs / frameDelayUs;
            nextFrameTime = micros();
        }

        void togglePause()
        {
            paused = !paused;
            vs1053.clearBuffer();
            nextFrameTime = micros();
            if (paused)
            {
                // VS1053 has no pause command for an MP3 stream. Mute hides
                // samples already buffered inside the decoder immediately.
                vs1053.setMuted(true);
                return;
            }

            if (!seekAudioTo(currentTimeUs()))
            {
                DEBUG_PRINTLN_F("VP resume without audio after pause");
                vs1053.setMuted(true);
                return;
            }
            vs1053.setMuted(false);
        }

        bool renderFrame()
        {
            if ((currentFrame + 1) * 1024UL > videoDataSize)
            {
                DEBUG_PRINT_F("VP frame out of range: ");
                DEBUG_PRINT(currentFrame);
                DEBUG_PRINT_F(" / ");
                DEBUG_PRINTLN(totalFrames());
                return false;
            }

            uint32_t videoPosition = 7 + currentFrame * 1024UL;
            bool positioned = false;
            size_t read = 0;
            for (uint8_t attempt = 0; attempt < 2; attempt++)
            {
                {
                    SchedulerSuspender lock;
                    positioned = videoData.seek(videoPosition);
                    read = positioned ? videoData.read(lcd.getFrameBuffer(), 1024) : 0;
                }
                if (positioned && read == 1024)
                    break;

                DEBUG_PRINT_F("VP video retry, frame=");
                DEBUG_PRINT(currentFrame);
                DEBUG_PRINT_F(" attempt=");
                DEBUG_PRINTLN(attempt + 1);
            }
            if (!positioned)
            {
                DEBUG_PRINT_F("VP video seek fail, frame=");
                DEBUG_PRINT(currentFrame);
                DEBUG_PRINT_F(" pos=");
                DEBUG_PRINT(videoPosition);
                DEBUG_PRINT_F(" size=");
                DEBUG_PRINTLN((uint32_t)videoData.fileSize());
                return false;
            }

            if (read != 1024)
            {
                DEBUG_PRINT_F("Frame read error: got ");
                DEBUG_PRINT(read);
                DEBUG_PRINT_F(" bytes, expected 1024, frame ");
                DEBUG_PRINTLN(currentFrame);
                return false;
            }

            currentFrame++;
            lcd.repaint();
            return true;
        }

        void renderPausedFrame()
        {
            if (!videoData || videoDataSize < 1024)
                return;

            uint32_t frame = currentFrame ? currentFrame - 1 : 0;
            if ((frame + 1) * 1024UL > videoDataSize)
                return;

            uint32_t savedPosition;
            size_t read;
            {
                SchedulerSuspender lock;
                savedPosition = videoData.curPosition();
                bool positioned = videoData.seek(7 + frame * 1024UL);
                read = positioned ? videoData.read(lcd.getFrameBuffer(), 1024) : 0;
                videoData.seek(savedPosition);
            }
            if (read == 1024)
                lcd.repaint();
        }

        void feedAudio()
        {
            if (!audioData || audioData.curPosition() >= audioData.fileSize())
                return;
            vs1053.feedFile(audioData);
        }

        void skipDueFrames()
        {
            uint32_t now = micros();
            while ((int32_t)(now - nextFrameTime) >= 0 &&
                   (currentFrame + 1) * 1024UL <= videoDataSize)
            {
                currentFrame++;
                nextFrameTime += frameDelayUs;
            }
        }

        void cleanup()
        {
            pendingVideoOpen = false;
            closeStartForm();
            removeVolumeOverlay();
            removeSeekOverlay();
            closePlaybackFile();
            if (audioAcquired)
            {
                vs1053.release();
                audioAcquired = false;
            }
        }

        void chooseVideoFile()
        {
            char* pathBuffer = (char*)pvPortMalloc(MAX_PATH_LEN);
            if (!pathBuffer)
            {
                ui.messageBox(F("Out of memory"));
                return;
            }

            pathBuffer[0] = '\0';
            DialogResult result = ui.openDialog(pathBuffer, MAX_PATH_LEN, TextRef(".vid"));
            
            if (result == dialogOk)
            {
                if (!startPlayback(pathBuffer))
                {
                    closePlaybackFile();
                    ui.messageBox(F("Cannot open video"));
                }
            }
            
            vPortFree(pathBuffer);
        }

        void openSelectedVideo()
        {
            // Больше не используется - открытие происходит сразу в chooseVideoFile
            pendingVideoOpen = false;
        }

        void onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            (void)value;
            if (event != EVENT_CLICK)
                return;

            if (sender == openButton)
                chooseVideoFile();
            else if (sender == exitButton)
                requestClose();
        }

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper *>(caption); }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }
        static bool canOpen(const char *extension)
        {
            return extension && extension[0] == '.' &&
                   tolower((unsigned char)extension[1]) == 'v' &&
                   tolower((unsigned char)extension[2]) == 'i' &&
                   tolower((unsigned char)extension[3]) == 'd' &&
                   extension[4] == '\0';
        }

        bool openFile(const char* path) override
        {
            return startPlayback(path);
        }

        ~VideoPlayer() override { cleanup(); }

        void fullRepaint() override
        {
            if (playing && foreground && !paused && currentFrame > 0)
            {
                currentFrame--;
                renderFrame();
            }
        }
        void onForeground() override
        {
            foreground = true;
            if (paused)
                renderPausedFrame();
        }
        void onBackground() override { foreground = false; }

    protected:
        void onClosing() override { cleanup(); }

        void setup() override
        {
            if (!vs1053.acquire())
            {
                requestClose();
                return;
            }
            audioAcquired = true;

            if (playing)
            {
                vs1053.setMuted(false);
                return;
            }

            startForm = new StartForm();
            if (!startForm)
            {
                cleanup();
                requestClose();
                return;
            }
            startForm->setCaption(getTaskCaption());
            startForm->setDrawBorder(true);
            openButton = startForm->addIconButton(40, 18, 20, 20, openIcon, 18, 18);
            exitButton = startForm->addIconButton(68, 18, 20, 20, exitIcon, 18, 18);
            if (!openButton || !exitButton)
            {
                cleanup();
                requestClose();
                return;
            }
            openButton->setEventMask(EVENT_CLICK);
            exitButton->setEventMask(EVENT_CLICK);
            ui.showForm(startForm);
        }

        void loop() override
        {
            if (pendingVideoOpen)
                openSelectedVideo();

            if (!playing)
            {
                if (startForm)
                {
                    taskYIELD();
                    return;
                }

                requestClose();
                return;
            }

            closeStartForm();

            bool spacePressed = keyboard.isKeyPressed(keySpace);
            if (spacePressed && !spaceWasPressed)
                togglePause();
            spaceWasPressed = spacePressed;

            if (keyboard.take(keyUp))
            {
                vs1053.increaseVolume();
                showVolumeOverlay();
            }
            if (keyboard.take(keyDown))
            {
                vs1053.decreaseVolume();
                showVolumeOverlay();
            }

            bool leftPressed = keyboard.isKeyPressed(keyLeft);
            bool rightPressed = keyboard.isKeyPressed(keyRight);
            int8_t direction = leftPressed ? -1 : (rightPressed ? 1 : 0);
            if (direction != 0)
            {
                if (!seeking || direction != seekDirection)
                    beginSeek(direction);
                updateSeek();
            }
            else if (seeking)
            {
                finishSeek();
            }

            updateOverlays();
            if (!paused && !seeking)
            {
                feedAudio();
                if (foreground)
                {
                    if ((int32_t)(micros() - nextFrameTime) >= 0)
                    {
                        if (!renderFrame())
                        {
                            cleanup();
                            requestClose();
                            return;
                        }
                        nextFrameTime += frameDelayUs;
                    }
                }
                else
                {
                    skipDueFrames();
                }
            }

            if (keyboard.take(keyEsc))
            {
                cleanup();
                requestClose();
                return;
            }

            taskYIELD();
        }
    };
}

#endif
