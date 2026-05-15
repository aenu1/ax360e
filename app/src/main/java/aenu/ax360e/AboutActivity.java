// SPDX-License-Identifier: WTFPL
package aenu.ax360e;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.webkit.WebView;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

public class AboutActivity extends AppCompatActivity {


    public static String get_update_log(){
        final String log="\n"
                +"0.2(2025-09-30)\n"
                + " *首个正式版本\n"
                +"0.3(2025-10-03)\n"
                + " *修正虚拟键盘闪退\n"
                + " *修正摇杆无效\n"
                + " *优化stfs格式识别（无后缀名即可）\n"
                + " *加入设置界面（暂不可用）\n"
                +"0.4(2025-10-07)\n"
                + " *完善设置\n"
                + " *添加虚拟键盘编辑\n"
                + " *多语言支持\n"
                +"0.6(2025-10-29)\n"
                + " *将apu,gpu,kernel等部分切换到xenia canary\n"
                +"0.8(2025-11-21)\n"
                + " *修正默认配置\n"
                + " *更新设置\n"
                +"0.9(2025-12-13)\n"
                + " *修正按键映射\n"
                + " *优化界面\n"
                +"0.10(2026-01-04)\n"
                + " *虚拟键盘优化\n"
                + " *将vfs部分切换到xenia canary\n"
                + " *设置完善\n"
                +"0.11(2026-01-09)\n"
                + " *修正了a64后端的部分实现\n"
                +"0.12(2026-01-31)\n"
                + " *部分修正\n"
                //+ " *创建快捷方式\n"
                +"0.13(2026-02-11)\n"
                + " *部分修正\n"
                +"0.14(2026-03-11)\n"
                + " *部分优化与修正\n"
                + " *修复手柄摇杆\n"
                + " *修复xex格式支持\n"
                + " *添加zar格式支持\n"
                +"1.15(2026-04-01)\n"
                + " *部分优化与修正\n"
                + " *适配不支持触屏的设备\n"
                + " *自定义驱动支持(仅adreno)\n"
                +"1.16(2026-05-15)\n"
                + " *切换至xenia canary\n"
                + " *部分修正\n"
                + " \n";

        return log;
    }

    TextView text;
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_about);
        
        // Setup Toolbar
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if(getSupportActionBar() != null) {
            getSupportActionBar().setTitle(R.string.about);
        }
        
        text=findViewById(R.id.about_text);
        findViewById(R.id.gratitude).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                text.setText(R.string.gratitude_content);
            }
        });
        findViewById(R.id.update_log).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                text.setText(get_update_log());
            }
        });

        findViewById(R.id.open_source_licenses).setOnClickListener(new View.OnClickListener() {
            void show_licenses_dialog(){
                AlertDialog.Builder ab=new AlertDialog.Builder(AboutActivity.this);
                ab.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface p1, int p2) {
                        p1.cancel();
                    }
                });
                WebView wv=new WebView(AboutActivity.this);
                wv.loadUrl("file:///android_asset/licenses.html");
                ab.setView(wv);
                ab.create().show();
            }
            @Override
            public void onClick(View v) {
                show_licenses_dialog();
            }
        });

        text.setText(Emulator.get.simple_device_info());
        text.setTextIsSelectable(true);
        text.setLongClickable(true);
    }
}
