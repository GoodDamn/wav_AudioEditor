#include <iostream>
#include <fstream>
#include <cmath>

class Log {
public:
	Log(const char* st) {
		std::cout << "\n" << st;
	}
};

class ShortLog
	: public Log {
public:
	ShortLog(const char* st, void* val) : Log(st) {
		std::cout << "<- " << *((int*)val) << "\n";
	}
};

class FullLog:
	public Log {
public:
	FullLog(const char* st) : Log(st) {
		std::cout << "<- LOG:\n";
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

class FileFactory {
protected:
	std::ifstream* inputFile;
	std::ofstream* outFile;

	void writeToFile(std::ofstream* stream, int value, int size) {
		stream->write(reinterpret_cast<const char*> (&value), size);
	}

	virtual void save(const char* st) {
		// Empty...
	}

public:

	FileFactory() {
		// Инициализация файлового потока на чтение
		inputFile = new std::ifstream();
	}

	~FileFactory() {
		// При вызове деструктора, очистить RAM
		delete inputFile;
		if (outFile != nullptr)
			delete outFile;
	}

	void saveFile(const char *st) {
		if (outFile == nullptr)
			outFile = new std::ofstream();
		save(st);
		outFile->close();
		inputFile->close();
	}
};

class AudioEditor : public FileFactory {
public:
	class Info {
	public:
		float duration = 0.0f; // Продолжительность
		float volume = 1.0f; // Общий уровень громкости
		short channels = 1; // Кол-во каналов
		short bitDepth = 8; // Глубина кодирования
		short formatChunkSize = 16; // Размер format-секции
		short compressionCode = 1; // Код компресии
		short blockAlign = bitDepth / 8; // Блок-канал
		int fileSize = 0; // Размер файла в байтах
		int dataSize = 0; // Размер аудиоданных в байтах
		int sampleRate = 48000; // Частота дискретизации
		int bitRate = sampleRate * bitDepth / 8; // Частота чтения битов
	};

	class Configuration {
	public:
		int fadeIn = 0; // Продолжительность плавного начала песни (в миллисекундах)
		int fadeOut = 0; // Продолжительность плавного оконачания песни (в миллисекундах)
		int startPosition = 0; // Начальная позиция обрезки песни (в миллисекундах)
		int endPosition = -1; // Конечная позиция обрезки песни (в миллисекундах)
		float playbackSpeed = 1.0f; // Скорость воспроизведения 
		short repeats = 1; // Эффект копирования (Число повторений), полезен для "зацикленных" композиций
		unsigned short compressSamples = 2; // Уровень сжатия
	};

	Info info;


	// Интефейсный метод для чтения файла
	void setDataSource(const char* path, Info* info, std::ifstream* inputFile) { 
		setData(path, info, inputFile); 
	}
	void setDataSource(const char* path) {
		setData(path, &info, inputFile);
	}

	void setOutputConfiguration(Configuration configuration) {
		this->mConfiguration = configuration;
	}

	void printInfo() { printInformation(&info); }
	void printInfo(Info* info) { printInformation(info); }

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
			float from = getSample(input, block, maxAmp);
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

	// Сохранение файла и применение изменений в содержимое файла
	
private:

	Configuration mConfiguration;

	short getDigitalSample(std::ifstream* stream, int size) {
		int s;
		stream->read((char*)&s, size);
		return s;
	}

	float getSample(std::ifstream* stream, int size, float maxAmp) {
		return (float)getDigitalSample(stream, size) / maxAmp;
	}

	void printInformation(Info* info) {
		std::cout << "\n\nFILE SIZE: " << info->fileSize
			<< "\nFORMAT CHUNK SIZE: " << info->formatChunkSize
			<< "\nCOMPRESSION CODE: " << info->compressionCode
			<< "\nCHANNELS: " << info->channels
			<< "\nSAMPLE RATE PER SECOND (Hz): " << info->sampleRate
			<< "\nBITRATE PER SECOND: " << info->bitRate
			<< "\nBLOCK OFFSET: " << info->blockAlign
			<< "\nBIT DEPTH: " << info->bitDepth
			<< "\nDURATION IN SEC.: " << info->duration
			<< "\nDATA SIZE:" << info->dataSize << "\n\n";
	}

	// Написание конфигурации о файле с указание структуры информации, потока для сохранения, пути для сохранения
	void writeInformation(Info* info, std::ofstream* audioFile, const char* path) {
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

	// Чтение данных с файла(путь, запись в структуры, поток чтения)
	void setData(const char* path, Info* info, std::ifstream* inputFile) {
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

protected:
	void save(const char* path) override {
		// Записываем параметры файла
		writeInformation(&info, outFile, path);

		short block = info.bitDepth / 8;

		unsigned int samplesFadeIn = (float)mConfiguration.fadeIn / 1000 * info.sampleRate; // 1500 ms
		unsigned int samplesFadeOut = (float)mConfiguration.fadeOut / 1000 * info.sampleRate; // 3500 ms
		unsigned int totalSamples = info.sampleRate * info.duration * info.blockAlign / info.channels;
		unsigned int trimmedSamplesFromEnd = (float)mConfiguration.endPosition / 1000 * info.sampleRate;

		FullLog("TOTAL INFO DURING SAVING PROCESS:");
		ShortLog("\tSAMPLE RATE:", &info.sampleRate);
		ShortLog("\tDURATION:", &info.duration);
		ShortLog("\tBLOCK OFFSET:", &info.blockAlign);
		ShortLog("\tCHANNELS:", &info.channels);
		ShortLog("\tTOTAL SAMPLES:", &totalSamples);

		totalSamples = totalSamples - samplesFadeOut - trimmedSamplesFromEnd;

		ShortLog("TOTAL SAMPLES WITH TRIMMING:", &totalSamples);

		float maxAmplitude = pow(2, info.bitDepth - 1) - 1;

		int startPosition = mConfiguration.startPosition / 1000 * info.sampleRate;
		totalSamples -= startPosition;
		int preAudioPos = outFile->tellp();
		int bytePos = startPosition * block + inputFile->tellg();
		inputFile->seekg(bytePos);

		ShortLog("FADE IN SAMPLES: ", &samplesFadeIn);
		ShortLog("FADE OUT SAMPLES:", &samplesFadeOut);
		ShortLog("TRIMMED SAMPLES FROM END:", &trimmedSamplesFromEnd);
		ShortLog("BLOCK OFFSET:", &block);

		short rep = 0;
		do {

			ShortLog("REPEATION", &rep);
			ShortLog("NUMBER OF SAMPLES: ", &samplesFadeIn);
			for (unsigned int i = 0; i < samplesFadeIn; i++) {
				int a;
				for (short i = 0; i < mConfiguration.compressSamples; i++)
					inputFile->read((char*)&a, block);
				if (inputFile->eof())
					break;
				float totalSample = getSample(inputFile, block, maxAmplitude) * Interpolator::cubic(i, samplesFadeIn) * info.volume;
				writeToFile(outFile, (int)(totalSample * maxAmplitude), block);
			}
			ShortLog("TOTAL SAMPLES:", &totalSamples);
			for (unsigned int i = samplesFadeIn; i < totalSamples; i++) {
				int a;
				for (short i = 0; i < mConfiguration.compressSamples; i++)
					inputFile->read((char*)&a, block);
				if (inputFile->eof())
					break;
				writeToFile(outFile, getSample(inputFile, block, maxAmplitude) * info.volume * maxAmplitude, block);
			}
			ShortLog("FADE OUT SAMPLES:",&samplesFadeOut);
			for (unsigned int i = 0; i < samplesFadeOut; i++) {
				int a;
				for (short i = 0; i < mConfiguration.compressSamples; i++)
					inputFile->read((char*)&a, block);
				if (inputFile->eof())
					break;
				float totalSample = getSample(inputFile, block, maxAmplitude) * (1.0f - Interpolator::linear(i, samplesFadeOut)) * info.volume;
				writeToFile(outFile, (int)(totalSample * maxAmplitude), block);
			}

			for (unsigned int i = 0; i < 25000; i++) {
				writeToFile(outFile, (int)(0.0f), block);
			}
			rep++;
			inputFile->seekg(bytePos);
		} while (rep < mConfiguration.repeats);

		int postAudioPos = outFile->tellp();

		outFile->seekp(preAudioPos - 4); // Data size
		writeToFile(outFile, postAudioPos - preAudioPos, 4);

		outFile->seekp(4, std::ios::beg); // File size
		writeToFile(outFile, postAudioPos - 8, 4);

		//uncompress(path);
	}

};

int main() {
	std::cout << "start parsing\n";

	AudioEditor* audioEditor = new AudioEditor();
	audioEditor->setDataSource("Hymn(48000).wav");

	AudioEditor::Configuration config;

	config.compressSamples = 22;

	audioEditor->printInfo();
	std::cout << "\n\n";
	audioEditor->info.bitDepth = 16;
	audioEditor->info.sampleRate = (audioEditor->info.sampleRate - audioEditor->info.sampleRate / config.compressSamples) / config.compressSamples; // 48000 Hz
	audioEditor->info.volume = 1.0f;

	config.repeats = 1;
	config.fadeIn = 1500; // 1500 ms
	config.fadeOut = 3500; // 3500 ms
	config.playbackSpeed = 1.0;
	config.startPosition = 0; // 9000 ms 
	config.endPosition = 0;

	audioEditor->setOutputConfiguration(config);

	audioEditor->saveFile("programWav.wav");

	FullLog("EXITING PROGRAM...");

	delete audioEditor;
	return 1;
}