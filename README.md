# AudioFormatDetective
Simple utility for AudioJungle reviewers
Uses audiotools and eyed3 to evaluate the attributes of audio files uploaded to the music library and checks them against the prerequisites. 

---

---

To run, install Docker (eg Docker desktop for macOS), then:

` docker run -it --rm --name afd -v <DOWNLOADS_FOLDER_PATH>:/AJTEMP sensedata1/audioformatdetective:prod `


It will wait for a zip to be downloaded, then:

- Unpack the zip
- Evaluate file formats displaying the results
- Do rudimentary watermark detection using speech recognition


