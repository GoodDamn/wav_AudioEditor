#include <iostream>
#include <fstream>
#include <cmath>

class Log {
public:
	Log(const char* st) {
		std::cout << "\n" << st << "\n";
	}
};

class Interpolator {
private:
	static float normalize(float val, int bound) {
		return val / bound;
	}

public:
	static float square(float value, int bound) {// Square parabola
		float val = normalize(value, bound);
		return val * val;
	}

	static float cubic(float value, int bound) { // Cubic parabola
		float val = normalize(value, bound);
		return val * val * val;
	}

	static float linear(float value, int bound) {
		return normalize(value, bound);
	}

	static float root(float value, int bound) { // (x)^(1/2)
		return sqrt(normalize(value, bound));
	}
};

class AudioEditor {
public:
	class Info {
	public:
		float duration = 0.0f;
		float volume = 1.0f;
		short channels = 1;
		short bitDepth = 8;
		short formatChunkSize = 16;
		short compressionCode = 1;
		short blockAlign = bitDepth / 8;
		int fileSize = 0, dataSize = 0,
			sampleRate = 48000, bitRate = sampleRate * bitDepth / 8;
	};

	class Configuration {
	public:
		int fadeIn = 0, fadeOut = 0;
		int startPosition = 0, endPosition = -1;
		float playbackSpeed = 1.0f;
		short repeats = 1;
		unsigned short compressSamples = 2;
	};

	Info info;

	std::ifstream* inputFile;
	std::ofstream* audioFile;

	AudioEditor() {
		inputFile = new std::ifstream();
	}

	~AudioEditor() {
		delete inputFile;
		if (audioFile != nullptr)
			delete audioFile;
	}

	void setDataSource(const char* path, Info* info, std::ifstream* inputFile) {setData(path,info,inputFile);}
	void setDataSource(const char* path) {setData(path, &info, inputFile);}

	void printInfo() { printInformation(&info); }
	void printInfo(Info* info) {printInformation(info);}

	void uncompress(const char* inPath) {
		std::ifstream* input = new std::ifstream();
		std::ofstream* audio = new std::ofstream();

		Info* info = new Info();
		setDataSource(inPath, info, input);
		std::cout << "\n\nuncompress::\n\n";
		printInfo(info);
		std::cout << "\n\nprocessing:: writing information\n\n";
		unsigned int totalSamples = info->sampleRate * info->duration * info->blockAlign / info->channels;
		info->sampleRate = info->sampleRate * 22;
		writeInformation(info, audio, "programWav_uncompressed.wav");

		printInformation(info);

		short block = info->blockAlign / 2;
		float maxAmp = pow(2, info->bitDepth - 1) - 1;

		int prePos = audio->tellp();

		
		unsigned int current = 0;
		std::cout << totalSamples << " - total samples\n\n";

		for (unsigned int c = 0; c < totalSamples; c++) {
			float from = getSample(input,block,maxAmp);
			int pos = input->tellg();
			float to = getSample(input, block, maxAmp);
			input->seekg(pos);
			float offset = (float)(to - from) / 22;

			for (short i = 0; i < 22; i++) {
				writeToFile(audio, from * maxAmp, block);
				from += offset;
			}
		}

		int postPos = audio->tellp();

		info->dataSize = postPos - prePos;
		audio->seekp(prePos - 4); // Data size
		writeToFile(audio, info->dataSize, 4);

		info->fileSize = postPos - 8;
		audio->seekp(4, std::ios::beg); // File size
		writeToFile(audio, info->fileSize, 4);

		std::cout << "\n\Final information:\n";
		printInformation(info);

		audio->close();
		input->close();
		delete input, audio;
	}

	void saveFile(const char* path, Configuration config) {
		if (audioFile == nullptr)
			audioFile = new std::ofstream();

		writeInformation(&info, audioFile, path);

		short block = info.bitDepth / 8;

		unsigned int samplesFadeIn = (float)config.fadeIn / 1000 * info.sampleRate; // 1500 ms
		unsigned int samplesFadeOut = (float)config.fadeOut / 1000 * info.sampleRate; // 3500 ms
		unsigned int totalSamples = info.sampleRate * info.duration * info.blockAlign / info.channels;
		unsigned int trimmedSamplesFromEnd = (float)config.endPosition / 1000 * info.sampleRate;
		std::cout << "\nTTInfo: " << info.sampleRate << " " << info.duration << " " << info.blockAlign << " " << info.channels << " " << totalSamples;
		totalSamples = totalSamples - samplesFadeOut - trimmedSamplesFromEnd;
		std::cout << "\nTTSamples: " << totalSamples;

		float maxAmplitude = pow(2, info.bitDepth - 1) - 1;

		int startPosition = config.startPosition / 1000 * info.sampleRate;
		totalSamples -= startPosition;
		int preAudioPos = audioFile->tellp();
		int bytePos = startPosition * block + inputFile->tellg();
		inputFile->seekg(bytePos);

		std::cout << "\nFade in: " << samplesFadeIn
			<< "\nFade out: " << samplesFadeOut
			<< "\nTrimmedFromEnd: " << trimmedSamplesFromEnd
			<< "\nBlock: " << block;

		short rep = 0;
		do {
			std::cout << "\n" << rep << " " << samplesFadeIn << " ";
			for (unsigned int i = 0; i < samplesFadeIn; i++) {
				int a;
				for (short i = 0; i < config.compressSamples; i++)
					inputFile->read((char*)&a, block);
				if (inputFile->eof())
					break;
				float totalSample = getSample(inputFile, block, maxAmplitude) * Interpolator::cubic(i, samplesFadeIn) * info.volume;
				writeToFile(audioFile, (int)(totalSample * maxAmplitude), block);
			}
			std::cout << totalSamples << " ";
			for (unsigned int i = samplesFadeIn; i < totalSamples; i++) {
				int a;
				for (short i = 0; i < config.compressSamples; i++)
					inputFile->read((char*)&a, block);
				if (inputFile->eof())
					break;
				writeToFile(audioFile, getSample(inputFile, block, maxAmplitude) * info.volume * maxAmplitude, block);
			}
			std::cout << samplesFadeOut << "\n";
			for (unsigned int i = 0; i < samplesFadeOut; i++) {
				int a;
				for (short i = 0; i < config.compressSamples; i++)
					inputFile->read((char*)&a, block);
				if (inputFile->eof())
					break;
				float totalSample = getSample(inputFile, block, maxAmplitude) * (1.0f - Interpolator::linear(i, samplesFadeOut)) * info.volume;
				writeToFile(audioFile, (int)(totalSample * maxAmplitude), block);
			}

			for (unsigned int i = 0; i < 25000; i++) {
				writeToFile(audioFile, (int)(0.0f), block);
			}
			rep++;
			inputFile->seekg(bytePos);
		} while (rep < config.repeats);

		int postAudioPos = audioFile->tellp();

		audioFile->seekp(preAudioPos - 4); // Data size
		writeToFile(audioFile, postAudioPos - preAudioPos, 4);

		audioFile->seekp(4, std::ios::beg); // File size
		writeToFile(audioFile, postAudioPos - 8, 4);

		audioFile->close();
		inputFile->close();

		uncompress(path);
	}

private:
	void writeToFile(std::ofstream *stream, int value, int size) {
		stream->write(reinterpret_cast<const char*> (&value), size);
	}

	short getDigitalSample(std::ifstream *stream,int size) {
		int s;
		stream->read((char*)&s, size);
		return s;
	}

	float getSample(std::ifstream *stream, int size ,float maxAmp) {
		return (float)getDigitalSample(stream,size) / maxAmp;
	}

	void printInformation(Info *info) {
		std::cout << "\n\nFile size: " << info->fileSize
			<< "\nSize format chunk: " << info->formatChunkSize
			<< "\nCompresion code: " << info->compressionCode
			<< "\nChannels: " << info->channels
			<< "\nSample Rate per second (Hz): " << info->sampleRate
			<< "\nBitRate per second: " << info->bitRate
			<< "\nBlock align: " << info->blockAlign
			<< "\nBit depth: " << info->bitDepth
			<< "\nDuration: " << info->duration
			<< "\nData size:" << info->dataSize << "\n\n";
	}

	void writeInformation(Info *info, std::ofstream *audioFile, const char *path) {
		audioFile->open(path, std::ios::binary | std::ios::out);
		*audioFile << "RIFF";
		writeToFile(audioFile, info->fileSize, 4);
		*audioFile << "WAVEfmt ";

		short block = info->bitDepth / 8;

		info->blockAlign = block * info->channels;
		info->bitRate = info->sampleRate * info->blockAlign;

		writeToFile(audioFile, 16, 4); // format chunk size
		writeToFile(audioFile, info->compressionCode, 2);
		writeToFile(audioFile, info->channels, 2);
		writeToFile(audioFile, info->sampleRate, 4);
		writeToFile(audioFile, info->bitRate, 4); // sampleRate * bitDepth / 8 * channels
		writeToFile(audioFile, info->blockAlign, 2); // bitDepth / 8 * channels
		writeToFile(audioFile, info->bitDepth, 2);
		*audioFile << "data";
		writeToFile(audioFile, info->dataSize, 4);
	}

	void setData(const char* path, Info* info, std::ifstream *inputFile) {
		inputFile->open(path, std::ios::in | std::ios::binary);
		if (!inputFile->is_open()) {
			std::cout << "File doesn't exist\n";
			return;
		}

		char riff[5];
		riff[4] = '\0';

		inputFile->read(reinterpret_cast<char*>(&riff), 4);
		inputFile->read(reinterpret_cast<char*>(&(info->fileSize)), 4);
		inputFile->seekg(16);
		inputFile->read(reinterpret_cast<char*>(&(info->formatChunkSize)), 4);
		inputFile->read((char*)&(info->compressionCode), 2);
		inputFile->read((char*)&(info->channels), 2);
		inputFile->read((char*)&(info->sampleRate), 4);
		inputFile->read((char*)&(info->bitRate), 4);
		inputFile->read((char*)&(info->blockAlign), 2);
		inputFile->read((char*)&(info->bitDepth), 2);

		char dataTag;
		char t[4];

		t[3] = '\0';
		while (1) {
			inputFile->read((char*)&dataTag, 1);
			if (dataTag == 'd') {
				inputFile->read((char*)&t, 3);
				if (t[0] == 'a' && t[1] == 't' && t[2] == 'a')
					break;
			}
		}
		inputFile->read((char*)&(info->dataSize), 4);
		info->duration = (float)info->dataSize / info->bitRate;
	}

};

int main() {
	std::cout << "start parsing\n";

	AudioEditor *audioEditor = new AudioEditor();
	audioEditor->setDataSource("Hymn(48000).wav");

	AudioEditor::Configuration config;

	config.compressSamples = 22;

	audioEditor->printInfo();
	std::cout << "\n\n";
	audioEditor->info.bitDepth = 16;
	audioEditor->info.sampleRate = (audioEditor->info.sampleRate - audioEditor->info.sampleRate/ config.compressSamples)/ config.compressSamples; // 48000 Hz
	audioEditor->info.volume = 1.0f;

	config.repeats = 1;
	config.fadeIn = 1500; // 1500 ms
	config.fadeOut = 3500; // 3500 ms
	config.playbackSpeed = 1.0;
	config.startPosition = 0; // 9000 ms 
	config.endPosition = 0;

	audioEditor->saveFile("programWav.wav",config);

	delete audioEditor;
	return 1;
}