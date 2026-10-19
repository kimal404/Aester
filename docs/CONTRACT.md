# Kontrak Antarmuka CLI aesterctl (CONTRACT_VERSION = 1)

Binary dijalankan via root:
su -c /data/adb/modules/aester/bin/aesterctl <subperintah>

1. Perintah dan Format Keluaran:
- aesterctl version: "<versi modul> <contract version>" (contoh: 1.0.0 1)
- aesterctl cpufreq get: persentase limit yang aktif (contoh: 70 atau off)
- aesterctl cpufreq set <40|50|60|70|80|90|off>: nilai yang berhasil diterapkan (pada dry-run: "dry-run <nilai>")
- aesterctl governor list: satu governor per baris (irisan semua policy)
- aesterctl governor get: nama governor aktif
- aesterctl governor set <nama>: nama governor yang berhasil diterapkan (pada dry-run: "dry-run <nama>")
- aesterctl governor reset: memulihkan governor ke nilai bawaan
- aesterctl io list: satu scheduler per baris (irisan semua block device terurut alfabet, melewati loop*, ram*, zram*, dm-*, mtdblock*)
- aesterctl io get: nama scheduler aktif dari primary block device
- aesterctl io set <nama>: nama scheduler yang berhasil diterapkan (pada dry-run: "dry-run <nama>")
- aesterctl io reset: memulihkan scheduler ke nilai bawaan
- aesterctl apply-saved: menerapkan seluruh konfigurasi tersimpan saat boot
- aesterctl status: format standar key=value satu pasangan per baris:
    cpufreq.requested=<nilai|off>
    cpufreq.<policy>.max=<aktual_khz>
    governor.requested=<nilai|->
    governor.<policy>=<aktual_gov>
    io.requested=<nilai|->
    io.<dev>=<aktual_sched>
- aesterctl status --pretty: format dashboard ringkasan manusiawi untuk display teks visual

2. Perilaku Khusus & Error Handling:
- Dry-run (AESTER_DRYRUN=1): operasi penulisan hanya dicatat ke debug log, verifikasi baca-ulang dilewati, tidak mengubah sysfs/state, menghasilkan stdout "dry-run <target>", exit 0.
- Penulisan Kolektif & Rollback: perintah set menulis ke seluruh target sebelum membaca ulang. Jika verifikasi salah satu target gagal, pemulihan best-effort dijalankan mengembalikan seluruh target ke nilai awal sebelum penulisan, menghasilkan exit 4, dan mencetak pesan stderr yang mencantumkan seluruh target yang gagal.

3. Exit Codes:
0: Ok, 1: General error, 2: Invalid args, 3: Not supported, 4: Write/Verify failed
