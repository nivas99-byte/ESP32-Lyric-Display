package com.example.lyricsender

import android.app.Notification
import android.content.Intent
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import okhttp3.Call
import okhttp3.Callback
import okhttp3.FormBody
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import java.io.IOException

class MusicListenerService : NotificationListenerService() {

    private val client = OkHttpClient()
    private val TAG = "MusicListenerService"

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        super.onNotificationPosted(sbn)
        
        val packageName = sbn?.packageName ?: return
        if (packageName != "com.google.android.apps.youtube.music") return

        val extras = sbn.notification.extras
        val title = extras.getString(Notification.EXTRA_TITLE) ?: ""
        val artist = extras.getString(Notification.EXTRA_TEXT) ?: ""
        val lyrics = extras.getCharSequence(Notification.EXTRA_SUB_TEXT)?.toString() ?: ""

        if (title.isNotEmpty()) {
            sendToESP32(title, artist, lyrics)
            
            // Broadcast to UI
            val intent = Intent("com.example.lyricsender.UPDATE_UI")
            intent.setPackage(this.packageName)
            intent.putExtra("title", title)
            intent.putExtra("artist", artist)
            intent.putExtra("lyrics", lyrics)
            sendBroadcast(intent)
        }
    }

    private fun sendToESP32(title: String, artist: String, lyrics: String) {
        val prefs = getSharedPreferences("MusicSenderPrefs", MODE_PRIVATE)
        val ip = prefs.getString("esp32_ip", "") ?: ""
        
        if (ip.isEmpty()) return

        val url = "http://$ip/update"
        val formBody = FormBody.Builder()
            .add("title", title)
            .add("artist", artist)
            .add("lyrics", lyrics)
            .build()

        val request = Request.Builder()
            .url(url)
            .post(formBody)
            .build()

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                Log.e(TAG, "Failed to send to ESP32: ${e.message}")
            }

            override fun onResponse(call: Call, response: Response) {
                response.use {
                    if (!it.isSuccessful) {
                        Log.e(TAG, "Unexpected response: $it")
                    }
                }
            }
        })
    }
}
