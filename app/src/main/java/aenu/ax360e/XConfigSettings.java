// SPDX-License-Identifier: WTFPL
package aenu.ax360e;

import android.content.Context;
import android.os.Bundle;
import android.view.Window;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.preference.CheckBoxPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;
import androidx.preference.SeekBarPreference;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Arrays;

/**
 * 编辑 storage_root/xconfig.settings (Xenia kernel::XConfigData 的裸二进制镜像)。
 * 该文件由 C++ 端 kernel_state 通过 fread/fwrite 直接读写, 结构为 #pragma pack(1)、
 * 大端序, 总大小 6680 字节。本界面通过 PreferenceDataStore 按字节偏移读写关键字段,
 * 功能对应桌面版 console_settings_dialog。
 */
public class XConfigSettings extends AppCompatActivity {

    // ---- XConfigData 结构大小 (各 block 顺序紧凑排列, 已由 offsetof 交叉验证) ----
    static final int FILE_SIZE = 6680;

    // ---- Secured block (base = 1766) ----
    static final int OFF_MAC_ADDRESS   = 1798; // +32, 6 bytes
    static final int OFF_AV_REGION     = 1806; // +40, be<uint32_t>

    // ---- User block (base = 2278) ----
    static final int OFF_LANGUAGE      = 2322; // +44, be<uint32_t>
    static final int OFF_VIDEO_FLAGS   = 2326; // +48, be<uint32_t>
    static final int OFF_AUDIO_FLAGS   = 2330; // +52, be<uint32_t>
    static final int OFF_RETAIL_FLAGS  = 2334; // +56, be<uint32_t>
    static final int OFF_COUNTRY       = 2342; // +64, be<uint8_t>
    static final int OFF_PC_FLAGS      = 2343; // +65, be<uint8_t>
    static final int OFF_AV_PACK_HDMI  = 2626; // +0x15C, be<int32_t>
    static final int OFF_PC_GAME       = 2638; // +0x168, be<uint32_t>
    static final int OFF_MUSIC_VOLUME  = 2727; // +0x1C1, be<float>

    // ---- 位掩码 ----
    static final long MASK_DOLBY_PRO_LOGIC = 0x00000001L;
    static final long MASK_ANALOG_MONO     = 0x00000002L;
    static final long MASK_DOLBY_DIGITAL   = 0x00010000L;
    static final long MASK_WMA_PRO         = 0x00030000L;
    static final long MASK_LOW_LATENCY     = 0x80000000L;
    static final long MASK_WIDESCREEN      = 0x00010000L;

    // ---- 默认值 (对应 C++ XConfig::SetDefaults) ----
    static final long DEF_AV_REGION    = 0x00400100L; // X_AV_REGION::NTSCM
    static final long DEF_LANGUAGE     = 1L;           // English
    static final long DEF_COUNTRY      = 103L;         // United States
    static final long DEF_AUDIO_FLAGS  = 0x00010001L;  // DolbyDigital | DolbyProLogic
    static final long DEF_RETAIL_FLAGS = 0x00000040L;  // DashboardInitialized
    static final long DEF_PC_FLAGS     = 0x03L;        // XBLAllowed | XBLMembershipCreationAllowed
    static final long DEF_PC_GAME      = 0xFFL;        // NoGameRestrictions
    static final long DEF_AV_HDMI      = 0x050002D0L;  // 1280x720
    static final float DEF_MUSIC_VOL   = 0.7f;

    static File get_xconfig_file() {
        return new File(Application.get_xe_storage_root(), "xconfig.settings");
    }

    // ===================== 二进制数据存取 =====================
    static class Store extends PreferenceDataStore {

        final File file;
        byte[] data;

        Store(File file) {
            this.file = file;
            load();
        }

        void load() {
            if (file.exists() && file.length() > 0) {
                int size = (int) Math.max(file.length(), FILE_SIZE);
                data = new byte[size];
                try (FileInputStream in = new FileInputStream(file)) {
                    int read = 0;
                    while (read < data.length) {
                        int n = in.read(data, read, data.length - read);
                        if (n < 0) break;
                        read += n;
                    }
                } catch (Exception e) {
                    data = new byte[FILE_SIZE];
                    set_defaults();
                }
            } else {
                data = new byte[FILE_SIZE];
                set_defaults();
                save();
            }
        }

        void save() {
            File parent = file.getParentFile();
            if (parent != null && !parent.exists()) parent.mkdirs();
            try (FileOutputStream out = new FileOutputStream(file)) {
                out.write(data);
                out.flush();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        void set_defaults() {
            Arrays.fill(data, (byte) 0);
            write_uint(OFF_AV_REGION, 4, DEF_AV_REGION);
            write_uint(OFF_LANGUAGE, 4, DEF_LANGUAGE);
            write_uint(OFF_COUNTRY, 1, DEF_COUNTRY);
            write_uint(OFF_AUDIO_FLAGS, 4, DEF_AUDIO_FLAGS);
            write_uint(OFF_VIDEO_FLAGS, 4, 0L);
            write_uint(OFF_RETAIL_FLAGS, 4, DEF_RETAIL_FLAGS);
            write_uint(OFF_PC_FLAGS, 1, DEF_PC_FLAGS);
            write_uint(OFF_PC_GAME, 4, DEF_PC_GAME);
            write_uint(OFF_AV_PACK_HDMI, 4, DEF_AV_HDMI);
            write_float(OFF_MUSIC_VOLUME, DEF_MUSIC_VOL);
        }

        long read_uint(int off, int width) {
            long v = 0;
            for (int i = 0; i < width; i++)
                v = (v << 8) | (data[off + i] & 0xFFL);
            return v;
        }

        void write_uint(int off, int width, long v) {
            for (int i = width - 1; i >= 0; i--) {
                data[off + i] = (byte) (v & 0xFFL);
                v >>>= 8;
            }
        }

        float read_float(int off) {
            return Float.intBitsToFloat((int) read_uint(off, 4));
        }

        void write_float(int off, float f) {
            write_uint(off, 4, Integer.toUnsignedLong(Float.floatToIntBits(f)));
        }

        void set_bit(int off, int width, long mask, boolean set) {
            long v = read_uint(off, width);
            if (set) v |= mask; else v &= ~mask;
            write_uint(off, width, v);
            save();
        }

        // ---------- PreferenceDataStore 分派 ----------
        @Nullable
        @Override
        public String getString(String key, @Nullable String defValue) {
            switch (key) {
                case "language":   return Long.toString(read_uint(OFF_LANGUAGE, 4));
                case "country":    return Long.toString(read_uint(OFF_COUNTRY, 1));
                case "av_region":  return Long.toString(read_uint(OFF_AV_REGION, 4));
                case "resolution": return Long.toString(read_uint(OFF_AV_PACK_HDMI, 4));
                case "mac_address":return bytes_to_hex(data, OFF_MAC_ADDRESS, 6);
            }
            return defValue;
        }

        @Override
        public void putString(String key, @Nullable String value) {
            if (value == null) return;
            try {
                switch (key) {
                    case "language":   write_uint(OFF_LANGUAGE, 4, Long.parseLong(value)); save(); break;
                    case "country":    write_uint(OFF_COUNTRY, 1, Long.parseLong(value)); save(); break;
                    case "av_region":  write_uint(OFF_AV_REGION, 4, Long.parseLong(value)); save(); break;
                    case "resolution": write_uint(OFF_AV_PACK_HDMI, 4, Long.parseLong(value)); save(); break;
                    case "mac_address":hex_to_bytes(value, data, OFF_MAC_ADDRESS, 6); save(); break;
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        @Override
        public boolean getBoolean(String key, boolean defValue) {
            switch (key) {
                case "widescreen":           return (read_uint(OFF_VIDEO_FLAGS, 4) & MASK_WIDESCREEN) != 0;
                case "audio_dolby_digital":  return (read_uint(OFF_AUDIO_FLAGS, 4) & MASK_DOLBY_DIGITAL) != 0;
                case "audio_dolby_pro_logic":return (read_uint(OFF_AUDIO_FLAGS, 4) & MASK_DOLBY_PRO_LOGIC) != 0;
                case "audio_mono":           return (read_uint(OFF_AUDIO_FLAGS, 4) & MASK_ANALOG_MONO) != 0;
                case "audio_wma_pro":        return (read_uint(OFF_AUDIO_FLAGS, 4) & MASK_WMA_PRO) != 0;
                case "audio_low_latency":    return (read_uint(OFF_AUDIO_FLAGS, 4) & MASK_LOW_LATENCY) != 0;
            }
            return defValue;
        }

        @Override
        public void putBoolean(String key, boolean value) {
            switch (key) {
                case "widescreen":           set_bit(OFF_VIDEO_FLAGS, 4, MASK_WIDESCREEN, value); break;
                case "audio_dolby_digital":  set_bit(OFF_AUDIO_FLAGS, 4, MASK_DOLBY_DIGITAL, value); break;
                case "audio_dolby_pro_logic":set_bit(OFF_AUDIO_FLAGS, 4, MASK_DOLBY_PRO_LOGIC, value); break;
                case "audio_mono":           set_bit(OFF_AUDIO_FLAGS, 4, MASK_ANALOG_MONO, value); break;
                case "audio_wma_pro":        set_bit(OFF_AUDIO_FLAGS, 4, MASK_WMA_PRO, value); break;
                case "audio_low_latency":    set_bit(OFF_AUDIO_FLAGS, 4, MASK_LOW_LATENCY, value); break;
            }
        }

        @Override
        public int getInt(String key, int defValue) {
            if (key.equals("music_volume"))
                return Math.round(read_float(OFF_MUSIC_VOLUME) * 100f);
            return defValue;
        }

        @Override
        public void putInt(String key, int value) {
            if (key.equals("music_volume")) {
                write_float(OFF_MUSIC_VOLUME, value / 100f);
                save();
            }
        }
    }

    static String bytes_to_hex(byte[] arr, int off, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            if (i > 0) sb.append(':');
            sb.append(String.format("%02X", arr[off + i] & 0xFF));
        }
        return sb.toString();
    }

    static void hex_to_bytes(String hex, byte[] arr, int off, int len) {
        String clean = hex.replaceAll("[^0-9A-Fa-f]", "");
        for (int i = 0; i < len; i++) {
            int idx = i * 2;
            int b = 0;
            if (idx + 2 <= clean.length())
                b = Integer.parseInt(clean.substring(idx, idx + 2), 16);
            arr[off + i] = (byte) b;
        }
    }

    // ===================== 选项数据 =====================
    //语言名称走 R.array.xcs_language_entries (多语言); 其余为区域无关的代码/数值
    static final String[] LANG_VALUES = {
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "11",
            "12", "13", "14", "15", "16", "17"};

    static final String[] COUNTRY_ENTRIES = {
            "AE","AL","AM","AR","AT","AU","AZ","BE","BG","BH","BN","BO","BR","BY","BZ",
            "CA","CH","CL","CN","CO","CR","CZ","DE","DK","DO","DZ","EC","EE","EG","ES",
            "FI","FO","FR","GB","GE","GR","GT","HK","HN","HR","HU","ID","IE","IL","IN",
            "IQ","IR","IS","IT","JM","JO","JP","KE","KG","KR","KW","KZ","LB","LI","LT",
            "LU","LV","LY","MA","MC","MK","MN","MO","MV","MX","MY","NI","NL","NO","NZ",
            "OM","PA","PE","PH","PK","PL","PR","PT","PY","QA","RO","RU","SA","SE","SG",
            "SI","SK","SV","SY","TH","TN","TR","TT","TW","UA","US","UY","UZ","VE","VN",
            "YE","ZA"};
    static final String[] COUNTRY_VALUES = {
            "1","2","3","4","5","6","7","8","9","10","11","12","13","14","15",
            "16","18","19","20","21","22","23","24","25","26","27","28","29","30","31",
            "32","33","34","35","36","37","38","39","40","41","42","43","44","45","46",
            "47","48","49","50","51","52","53","54","55","56","57","58","59","60","61",
            "62","63","64","65","66","67","68","69","70","71","72","73","74","75","76",
            "77","78","79","80","81","82","83","84","85","86","87","88","89","90","91",
            "92","93","95","96","97","98","99","100","101","102","103","104","105","106","107",
            "108","109"};

    static final String[] AVREGION_ENTRIES = {"NTSC", "NTSC-J", "PAL", "PAL 50Hz"};
    static final String[] AVREGION_VALUES  = {"4194560", "4194816", "4195328", "8389376"};

    static final String[] RES_ENTRIES = {"640x480", "1280x720", "1920x1080"};
    static final String[] RES_VALUES  = {"41943520", "83886800", "125830200"};

    // ===================== Fragment =====================
    public static class Frag extends PreferenceFragmentCompat {

        Store store;

        @Override
        public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
            store = new Store(get_xconfig_file());
            getPreferenceManager().setPreferenceDataStore(store);

            Context ctx = requireContext();
            PreferenceScreen root = getPreferenceManager().createPreferenceScreen(ctx);
            String[] lang_entries = getResources().getStringArray(R.array.xcs_language_entries);

            // ---- User ----
            PreferenceCategory cat_user = category(ctx, root, "cat_user", R.string.xcs_cat_user);
            cat_user.addPreference(list(ctx, "language", R.string.xcs_language, lang_entries, LANG_VALUES));
            cat_user.addPreference(list(ctx, "country", R.string.xcs_country, COUNTRY_ENTRIES, COUNTRY_VALUES));

            // ---- Video ----
            PreferenceCategory cat_video = category(ctx, root, "cat_video", R.string.xcs_cat_video);
            cat_video.addPreference(list(ctx, "av_region", R.string.xcs_av_region, AVREGION_ENTRIES, AVREGION_VALUES));
            cat_video.addPreference(list(ctx, "resolution", R.string.xcs_resolution, RES_ENTRIES, RES_VALUES));
            cat_video.addPreference(checkbox(ctx, "widescreen", R.string.xcs_widescreen));

            // ---- Audio ----
            PreferenceCategory cat_audio = category(ctx, root, "cat_audio", R.string.xcs_cat_audio);
            cat_audio.addPreference(checkbox(ctx, "audio_dolby_digital", R.string.xcs_dolby_digital));
            cat_audio.addPreference(checkbox(ctx, "audio_dolby_pro_logic", R.string.xcs_dolby_pro_logic));
            cat_audio.addPreference(checkbox(ctx, "audio_mono", R.string.xcs_analog_mono));
            cat_audio.addPreference(checkbox(ctx, "audio_wma_pro", R.string.xcs_wma_pro));
            cat_audio.addPreference(checkbox(ctx, "audio_low_latency", R.string.xcs_low_latency));

            SeekBarPreference vol = new SeekBarPreference(ctx);
            vol.setKey("music_volume");
            vol.setTitle(R.string.xcs_music_volume);
            vol.setIconSpaceReserved(false);
            vol.setMin(0);
            vol.setMax(100);
            vol.setShowSeekBarValue(true);
            cat_audio.addPreference(vol);

            // ---- Network ----
            PreferenceCategory cat_net = category(ctx, root, "cat_net", R.string.xcs_cat_network);
            EditTextPreference mac = new EditTextPreference(ctx);
            mac.setKey("mac_address");
            mac.setTitle(R.string.xcs_mac_address);
            mac.setIconSpaceReserved(false);
            mac.setDialogMessage(R.string.xcs_mac_format);
            mac.setSummaryProvider(preference ->
                    ((EditTextPreference) preference).getText());
            cat_net.addPreference(mac);

            setPreferenceScreen(root);
        }

        PreferenceCategory category(Context ctx, PreferenceScreen root, String key, int title) {
            PreferenceCategory c = new PreferenceCategory(ctx);
            c.setKey(key);
            c.setTitle(title);
            c.setIconSpaceReserved(false);
            root.addPreference(c);
            return c;
        }

        ListPreference list(Context ctx, String key, int title, String[] entries, String[] values) {
            ListPreference p = new ListPreference(ctx);
            p.setKey(key);
            p.setTitle(title);
            p.setIconSpaceReserved(false);
            p.setEntries(entries);
            p.setEntryValues(values);
            p.setSummaryProvider(ListPreference.SimpleSummaryProvider.getInstance());
            return p;
        }

        CheckBoxPreference checkbox(Context ctx, String key, int title) {
            CheckBoxPreference p = new CheckBoxPreference(ctx);
            p.setKey(key);
            p.setTitle(title);
            p.setIconSpaceReserved(false);
            return p;
        }
    }

    // ===================== Activity =====================
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);

        Utils.setup_system_bars_appearance(this);
        setContentView(R.layout.activity_emulator_settings);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null)
            getSupportActionBar().setTitle(getString(R.string.console_settings));
        toolbar.setNavigationOnClickListener(v -> onBackPressed());

        getSupportFragmentManager().beginTransaction()
                .replace(R.id.settings_container, new Frag())
                .commit();
    }
}
