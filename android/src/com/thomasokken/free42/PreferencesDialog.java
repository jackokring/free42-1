/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2011  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

package com.thomasokken.free42;

import android.app.Dialog;
import android.content.Context;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;

public class PreferencesDialog extends Dialog {
	private CheckBox singularMatrixCB;
	private CheckBox matrixOutOfRangeCB;
	private CheckBox autoRepeatCB;
	private CheckBox printToTextCB;
	private EditText printToTextFileNameTF;
	private CheckBox rawTextCB;
	private CheckBox printToGifCB;
	private EditText printToGifFileNameTF;
	private EditText maxGifHeightTF;
	private OkListener okListener;
	
	public PreferencesDialog(Context context) {
		super(context);
		setContentView(R.layout.preferences_dialog);
		singularMatrixCB = (CheckBox) findViewById(R.id.singularMatrixCB);
		matrixOutOfRangeCB = (CheckBox) findViewById(R.id.matrixOutOfRangeCB);
		autoRepeatCB = (CheckBox) findViewById(R.id.autoRepeatCB);
		printToTextCB = (CheckBox) findViewById(R.id.printToTextCB);
		Button browseTextB = (Button) findViewById(R.id.browseTextB);
		browseTextB.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				browseTextFileName(view.getContext());
			}
		});
		printToTextFileNameTF = (EditText) findViewById(R.id.printToTextFileNameTF);
		rawTextCB = (CheckBox) findViewById(R.id.rawTextCB);
		printToGifCB = (CheckBox) findViewById(R.id.printToGifCB);
		Button browseGifB = (Button) findViewById(R.id.browseGifB);
		browseGifB.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				browseGifFileName(view.getContext());
			}
		});
		printToGifFileNameTF = (EditText) findViewById(R.id.printToGifFileNameTF);
		maxGifHeightTF = (EditText) findViewById(R.id.maxGifHeightTF);
		Button okB = (Button) findViewById(R.id.okB);
		okB.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				if (okListener != null)
					okListener.okPressed();
				PreferencesDialog.this.hide();
			}
		});
		Button cancelB = (Button) findViewById(R.id.cancelB);
		cancelB.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				PreferencesDialog.this.hide();
			}
		});
		setTitle("Preferences");
	}
	
	public interface OkListener {
		public void okPressed();
	}

	public void setOkListener(OkListener okListener) {
		this.okListener = okListener;
	}
	
	private void browseTextFileName(Context context) {
		FileSelectionDialog fsd = new FileSelectionDialog(context, new String[] { "txt", "*" });
		fsd.setPath(printToTextFileNameTF.getText().toString());
		fsd.setOkListener(new FileSelectionDialog.OkListener() {
			public void okPressed(String path) {
				printToTextFileNameTF.setText(path);
			}
		});
		fsd.show();
	}
	
	private void browseGifFileName(Context context) {
		FileSelectionDialog fsd = new FileSelectionDialog(context, new String[] { "gif", "*" });
		fsd.setPath(printToGifFileNameTF.getText().toString());
		fsd.setOkListener(new FileSelectionDialog.OkListener() {
			public void okPressed(String path) {
				printToGifFileNameTF.setText(path);
			}
		});
		fsd.show();
	}
	
	public void setSingularMatrixError(boolean b) {
		singularMatrixCB.setChecked(b);
	}
	
	public boolean getSingularMatrixError() {
		return singularMatrixCB.isChecked();
	}
	
	public void setMatrixOutOfRange(boolean b) {
		matrixOutOfRangeCB.setChecked(b);
	}
	
	public boolean getMatrixOutOfRange() {
		return matrixOutOfRangeCB.isChecked();
	}
	
	public void setAutoRepeat(boolean b) {
		autoRepeatCB.setChecked(b);
	}
	
	public boolean getAutoRepeat() {
		return autoRepeatCB.isChecked();
	}
	
	public void setPrintToText(boolean b) {
		printToTextCB.setChecked(b);
	}
	
	public boolean getPrintToText() {
		return printToTextCB.isChecked();
	}
	
	public void setPrintToTextFileName(String s) {
		printToTextFileNameTF.setText(s);
	}
	
	public String getPrintToTextFileName() {
		return printToTextFileNameTF.getText().toString();
	}
	
	public void setRawText(boolean b) {
		rawTextCB.setChecked(b);
	}
	
	public boolean getRawText() {
		return rawTextCB.isChecked();
	}
	
	public void setPrintToGif(boolean b) {
		printToGifCB.setChecked(b);
	}
	
	public boolean getPrintToGif() {
		return printToGifCB.isChecked();
	}

	public void setPrintToGifFileName(String s) {
		printToGifFileNameTF.setText(s);
	}
	
	public String getPrintToGifFileName() {
		return printToGifFileNameTF.getText().toString();
	}

	public void setMaxGifHeight(int i) {
		maxGifHeightTF.setText(Integer.toString(i));
	}
	
	public int getMaxGifHeight() {
		try {
			int n = Integer.parseInt(maxGifHeightTF.getText().toString());
			if (n < 32)
				n = 32;
			else if (n > 32767)
				n = 32767;
			return n;
		} catch (NumberFormatException e) {
			return 256;
		}
	}
}
