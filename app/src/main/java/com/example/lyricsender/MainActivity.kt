package com.example.lyricsender

import android.content.BroadcastReceiver
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.edit
import com.example.lyricsender.ui.theme.LYRICSENDERTheme

class MainActivity : ComponentActivity() {

    private var lastSongInfo by mutableStateOf("No song intercepted yet")

    private val updateReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            val title = intent?.getStringExtra("title") ?: ""
            val artist = intent?.getStringExtra("artist") ?: ""
            val lyrics = intent?.getStringExtra("lyrics") ?: ""
            lastSongInfo = "Title: $title\nArtist: $artist\nLyrics: $lyrics"
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        
        setContent {
            LYRICSENDERTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    MainScreen(
                        lastSong = lastSongInfo,
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        val filter = IntentFilter("com.example.lyricsender.UPDATE_UI")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(updateReceiver, filter, RECEIVER_NOT_EXPORTED)
        } else {
            registerReceiver(updateReceiver, filter)
        }
    }

    override fun onPause() {
        super.onPause()
        unregisterReceiver(updateReceiver)
    }
}

@Composable
fun MainScreen(lastSong: String, modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val prefs = remember { context.getSharedPreferences("MusicSenderPrefs", Context.MODE_PRIVATE) }
    var ipAddress by remember { mutableStateOf(prefs.getString("esp32_ip", "") ?: "") }

    var isPermissionGranted by remember { mutableStateOf(isNotificationServiceEnabled(context)) }

    // Re-check permission when component is recomposed
    LaunchedEffect(Unit) {
        isPermissionGranted = isNotificationServiceEnabled(context)
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Text(text = "Lyrics Sender", style = MaterialTheme.typography.headlineMedium)

        OutlinedTextField(
            value = ipAddress,
            onValueChange = {
                ipAddress = it
                prefs.edit { putString("esp32_ip", it) }
            },
            label = { Text("ESP32 IP Address") },
            modifier = Modifier.fillMaxWidth(),
            placeholder = { Text("e.g. 192.168.1.50") },
            singleLine = true
        )

        Button(
            onClick = {
                val intent = Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
                context.startActivity(intent)
            },
            modifier = Modifier.fillMaxWidth(),
            colors = if (isPermissionGranted) ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.secondary) else ButtonDefaults.buttonColors()
        ) {
            Text(if (isPermissionGranted) "Permission Granted" else "Grant Notification Permission")
        }

        Spacer(modifier = Modifier.height(8.dp))

        Card(
            modifier = Modifier.fillMaxWidth().weight(1f),
            elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
        ) {
            Column(modifier = Modifier.padding(16.dp).verticalScroll(rememberScrollState())) {
                Text(text = "Status / Last Data:", style = MaterialTheme.typography.titleMedium)
                HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                Text(text = lastSong, style = MaterialTheme.typography.bodyMedium)
            }
        }
    }
}

fun isNotificationServiceEnabled(context: Context): Boolean {
    val pkgName = context.packageName
    val flat = Settings.Secure.getString(context.contentResolver, "enabled_notification_listeners")
    if (!flat.isNullOrEmpty()) {
        val names = flat.split(":")
        for (name in names) {
            val componentName = ComponentName.unflattenFromString(name)
            if (componentName != null) {
                if (pkgName == componentName.packageName) {
                    return true
                }
            }
        }
    }
    return false
}
