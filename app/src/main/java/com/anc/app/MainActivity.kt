package com.anc.app

import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioRecord
import android.media.AudioTrack
import android.media.MediaRecorder
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Switch
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : AppCompatActivity() {
    private lateinit var engine: NoiseReductionEngine
    private var audioThread: Thread? = null
    private val running = AtomicBoolean(false)

    private lateinit var btnToggle: Button
    private lateinit var seekGain: SeekBar
    private lateinit var seekGate: SeekBar
    private lateinit var seekHPF: SeekBar
    private lateinit var seekLPF: SeekBar
    private lateinit var txtGain: TextView
    private lateinit var txtGate: TextView
    private lateinit var txtHPF: TextView
    private lateinit var txtLPF: TextView
    private lateinit var switchAdaptive: Switch
    private lateinit var txtNoiseLevel: TextView
    private lateinit var txtCurrentGain: TextView
    private lateinit var txtInputDevice: TextView

    private var recorder: AudioRecord? = null
    private var player: AudioTrack? = null
    private val handler = Handler(Looper.getMainLooper())
    private var audioManager: AudioManager? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        engine = NoiseReductionEngine(this)
        audioManager = getSystemService(AUDIO_SERVICE) as AudioManager

        btnToggle = findViewById(R.id.btnToggle)
        seekGain = findViewById(R.id.seekGain)
        seekGate = findViewById(R.id.seekGate)
        seekHPF = findViewById(R.id.seekHPF)
        seekLPF = findViewById(R.id.seekLPF)
        txtGain = findViewById(R.id.txtGain)
        txtGate = findViewById(R.id.txtGate)
        txtHPF = findViewById(R.id.txtHPF)
        txtLPF = findViewById(R.id.txtLPF)
        switchAdaptive = findViewById(R.id.switchAdaptive)
        txtNoiseLevel = findViewById(R.id.txtNoiseLevel)
        txtCurrentGain = findViewById(R.id.txtCurrentGain)
        txtInputDevice = findViewById(R.id.txtInputDevice)

        setupControls()
        requestPermissions()
        registerBluetoothCallback()
    }

    private fun setupControls() {
        btnToggle.setOnClickListener {
            if (running.get()) {
                stopAudio()
                btnToggle.text = "Start"
            } else {
                if (checkPermission()) {
                    startAudio()
                    btnToggle.text = "Stop"
                } else {
                    requestPermissions()
                }
            }
        }

        switchAdaptive.isChecked = true
        switchAdaptive.setOnCheckedChangeListener { _, isChecked ->
            engine.setAdaptive(isChecked)
            seekGain.isEnabled = !isChecked
        }

        seekGain.max = 100
        seekGain.progress = 20
        seekGain.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(sb: SeekBar, p: Int, fromUser: Boolean) {
                val gain = p / 100f
                txtGain.text = "Gain: ${"%.2f".format(gain)}"
                engine.setGain(gain)
            }
            override fun onStartTrackingTouch(sb: SeekBar) {}
            override fun onStopTrackingTouch(sb: SeekBar) {}
        })

        seekGate.max = 200
        seekGate.progress = 10
        seekGate.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(sb: SeekBar, p: Int, fromUser: Boolean) {
                val gate = p / 1000f
                txtGate.text = "Gate: ${"%.3f".format(gate)}"
                engine.setGateThreshold(gate)
            }
            override fun onStartTrackingTouch(sb: SeekBar) {}
            override fun onStopTrackingTouch(sb: SeekBar) {}
        })

        seekHPF.max = 480
        seekHPF.progress = 180
        seekHPF.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(sb: SeekBar, p: Int, fromUser: Boolean) {
                val hpf = (p + 20).toFloat()
                txtHPF.text = "HPF: ${hpf.toInt()} Hz"
                engine.setHPFCutoff(hpf)
            }
            override fun onStartTrackingTouch(sb: SeekBar) {}
            override fun onStopTrackingTouch(sb: SeekBar) {}
        })

        seekLPF.max = 7500
        seekLPF.progress = 1500
        seekLPF.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(sb: SeekBar, p: Int, fromUser: Boolean) {
                val lpf = (p + 500).toFloat()
                txtLPF.text = "LPF: ${lpf.toInt()} Hz"
                engine.setLPFCutoff(lpf)
            }
            override fun onStartTrackingTouch(sb: SeekBar) {}
            override fun onStopTrackingTouch(sb: SeekBar) {}
        })

        // Start status update timer
        startStatusUpdates()
    }

    private fun startAudio() {
        val bufferSize = AudioRecord.getMinBufferSize(48000,
            AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_FLOAT) * 2

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
            != PackageManager.PERMISSION_GRANTED) return

        // Try to use Bluetooth mic if available
        val inputDevice = findBluetoothInputDevice()
        val deviceName = if (inputDevice != null) {
            "Bluetooth: ${inputDevice.productName}"
        } else {
            "Phone mic (no Bluetooth device found)"
        }
        txtInputDevice.text = "Input: $deviceName"

        val recorderBuilder = AudioRecord.Builder()
            .setAudioSource(MediaRecorder.AudioSource.MIC)
            .setAudioFormat(AudioFormat.Builder()
                .setSampleRate(48000)
                .setChannelMask(AudioFormat.CHANNEL_IN_MONO)
                .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                .build())
            .setBufferSizeInBytes(bufferSize)

        if (inputDevice != null) {
            recorderBuilder.setAudioSource(MediaRecorder.AudioSource.VOICE_COMMUNICATION)
            // Note: Android will automatically route to BT SCO when headset is connected
        }

        recorder = recorderBuilder.build()

        val outBufferSize = AudioTrack.getMinBufferSize(48000,
            AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_FLOAT)

        player = AudioTrack.Builder()
            .setAudioFormat(AudioFormat.Builder()
                .setSampleRate(48000)
                .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                .build())
            .setBufferSizeInBytes(outBufferSize)
            .setTransferMode(AudioTrack.MODE_STREAM)
            .build()

        recorder?.startRecording()
        player?.play()
        engine.setEnabled(true)

        running.set(true)
        audioThread = Thread {
            val buf = FloatArray(256)
            while (running.get()) {
                val read = recorder?.read(buf, 0, buf.size,
                    AudioRecord.READ_BLOCKING) ?: -1
                if (read > 0) {
                    player?.write(buf, 0, read, AudioTrack.WRITE_BLOCKING)
                }
            }
        }.also { it.start() }
    }

    private fun stopAudio() {
        running.set(false)
        engine.setEnabled(false)
        audioThread?.join(1000)
        recorder?.stop()
        recorder?.release()
        recorder = null
        player?.stop()
        player?.release()
        player = null
    }

    private fun checkPermission(): Boolean {
        return ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
            == PackageManager.PERMISSION_GRANTED
    }

    private fun requestPermissions() {
        ActivityCompat.requestPermissions(this,
            arrayOf(Manifest.permission.RECORD_AUDIO), 100)
    }

    private fun findBluetoothInputDevice(): AudioDeviceInfo? {
        val audioManager = getSystemService(AUDIO_SERVICE) as AudioManager
        val devices = audioManager.getDevices(AudioManager.GET_DEVICES_INPUTS)
        for (device in devices) {
            if (device.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO ||
                device.type == AudioDeviceInfo.TYPE_BLUETOOTH_A2DP) {
                return device
            }
        }
        return null
    }

    private fun registerBluetoothCallback() {
        audioManager?.registerAudioDeviceCallback(object : AudioDeviceCallback() {
            override fun onAudioDevicesAdded(addedDevices: Array<out AudioDeviceInfo>?) {
                super.onAudioDevicesAdded(addedDevices)
                // Check if Bluetooth device was added
                val btDevice = addedDevices?.find {
                    it.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO ||
                    it.type == AudioDeviceInfo.TYPE_BLUETOOTH_A2DP
                }
                if (btDevice != null && running.get()) {
                    // Restart audio to use new Bluetooth device
                    handler.post {
                        stopAudio()
                        startAudio()
                        btnToggle.text = "Stop"
                    }
                }
            }
        }, handler)
    }

    private fun startStatusUpdates() {
        handler.postDelayed(object : Runnable {
            override fun run() {
                if (running.get()) {
                    val noiseLevel = engine.getNoiseLevel()
                    val currentGain = engine.getCurrentGain()
                    txtNoiseLevel.text = "Noise Level: ${"%.3f".format(noiseLevel)}"
                    txtCurrentGain.text = "Current Gain: ${"%.2f".format(currentGain)}"
                }
                handler.postDelayed(this, 100)
            }
        }, 100)
    }

    override fun onDestroy() {
        super.onDestroy()
        if (running.get()) stopAudio()
        engine.destroy()
        handler.removeCallbacksAndMessages(null)
    }
}
