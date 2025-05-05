package com.example.ece420_project;

import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Arrays;

public class MainActivity extends AppCompatActivity {

    static {
        Log.d("JNI", "Loading native-lib...");
        System.loadLibrary("native-lib");
    }

    // Native functions
    public native void addTrainingExample(String label, short[] audio);
    public native String predictGender(short[] audio);

    private TextView resultTextView;
    private AudioRecord recorder;
    private boolean isRecording = false;
    private Thread recordingThread;

    private static final int SAMPLE_RATE = 16000;
    private static final int CHUNK_SIZE = SAMPLE_RATE;   // 1 sec
    private static final int HOP_SIZE = CHUNK_SIZE / 2;       // 50% overlap

    private ProgressBar loadingProgressBar;
    private TextView loadingTextView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        resultTextView = findViewById(R.id.resultTextView);

        Button recordButton = findViewById(R.id.recordButton);

        loadingProgressBar = findViewById(R.id.loadingProgressBar);
        loadingTextView = findViewById(R.id.loadingTextView);

// Show loading UI
        loadingProgressBar.setVisibility(View.VISIBLE);
        loadingTextView.setVisibility(View.VISIBLE);

// Hide main interaction until loading completes
        resultTextView.setVisibility(View.GONE);
        recordButton.setEnabled(false);

// Load data in a background thread
        new Thread(this::loadTrainingDataFromAssets).start();

        recordButton.setOnClickListener(v -> toggleRecording());
        recordButton.setText("Start Live Prediction");
    }

    private void toggleRecording() {
        Button recordButton = findViewById(R.id.recordButton);

        if (isRecording) {
            isRecording = false;
            recordButton.setText("Start Live Prediction");

            if (recordingThread != null) {
                try {
                    recordingThread.join();  // wait for thread to finish safely
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                recordingThread = null;
            }

            if (recorder != null) {
                recorder.stop();
                recorder.release();
                recorder = null;
            }

        } else {
            isRecording = true;
            recordButton.setText("Stop Prediction");
            startLivePrediction();
        }
    }

    private void startLivePrediction() {
        if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        recorder = new AudioRecord(
                MediaRecorder.AudioSource.MIC,
                SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                CHUNK_SIZE * 2  // bytes: 2 bytes per sample
        );

        recorder.startRecording();

        recordingThread = new Thread(() -> {
            short[] buffer = new short[CHUNK_SIZE];
            int offset = 0;

            while (isRecording && !Thread.currentThread().isInterrupted()) {
                int read = recorder.read(buffer, offset, CHUNK_SIZE - offset);
                if (read > 0) {
                    offset += read;
                    if (offset >= CHUNK_SIZE) {
                        short[] window = new short[CHUNK_SIZE];
                        System.arraycopy(buffer, 0, window, 0, CHUNK_SIZE);
                        String result = predictGender(window);
                        runOnUiThread(() -> resultTextView.setText("Prediction: " + result));

                        // Shift for 50% hop
                        System.arraycopy(buffer, HOP_SIZE, buffer, 0, CHUNK_SIZE - HOP_SIZE);
                        offset = CHUNK_SIZE - HOP_SIZE;
                    }
                }
            }
        });
        recordingThread.start();
    }


    private void loadTrainingDataFromAssets() {
        AssetManager am = getAssets();
        String[] genders = {"male", "female"};

        for (String gender : genders) {
            try {
                String basePath = "training/" + gender;
                String[] files = am.list(basePath);

                Log.d("AssetDebug", "Reading from: " + basePath);
                if (files == null || files.length == 0) {
                    Log.e("AssetDebug", "No files found in: " + basePath);
                    continue;
                }

                Log.d("AssetDebug", "Found files: " + Arrays.toString(files));

                for (String file : files) {
                    if (file.endsWith(".wav")) {
                        String assetPath = basePath + "/" + file;
                        short[] pcm = loadWavFromAssets(assetPath);
                        addTrainingExample(gender, pcm);
                        Log.d("AssetDebug", "Loaded file: " + assetPath);
                    }
                }
            } catch (IOException e) {
                Log.e("AssetDebug", "Failed to load training files for: " + gender, e);
            }
        }

        runOnUiThread(() -> {
            Toast.makeText(this, "All training data are loaded.", Toast.LENGTH_SHORT).show();
            loadingProgressBar.setVisibility(View.GONE);
            loadingTextView.setVisibility(View.GONE);
            resultTextView.setVisibility(View.VISIBLE);
            findViewById(R.id.recordButton).setEnabled(true);
        });
    }

    private short[] loadWavFromAssets(String path) throws IOException {
        AssetFileDescriptor afd = getAssets().openFd(path);
        FileInputStream inputStream = afd.createInputStream();
        inputStream.skip(44); // Skip WAV header

        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        byte[] temp = new byte[4096];
        int read;
        while ((read = inputStream.read(temp)) != -1) {
            buffer.write(temp, 0, read);
        }
        inputStream.close();
        byte[] audioBytes = buffer.toByteArray();

        short[] pcm = new short[audioBytes.length / 2];
        for (int i = 0; i < pcm.length; i++) {
            pcm[i] = (short) ((audioBytes[2 * i] & 0xFF) | (audioBytes[2 * i + 1] << 8));
        }
        return pcm;
    }
}
