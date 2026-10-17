"""Offline tests: no credentials, network calls or ElevenLabs credits are used."""
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError, URLError
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'assets/elevenlabs'))
import client

class ElevenLabsTests(unittest.TestCase):
    def test_key_file_and_environment(self):
        with tempfile.TemporaryDirectory() as work, patch.dict(os.environ, {}, clear=True):
            p = Path(work) / '.env.local'
            with self.assertRaises(client.AudioError): client.load_key(p)
            p.write_text('ELEVENLABS_API_KEY="local-test-key"\n')
            self.assertEqual(client.load_key(p), 'local-test-key')
            with patch.dict(os.environ, {'ELEVENLABS_API_KEY':'environment-test-key'}):
                self.assertEqual(client.load_key(p), 'environment-test-key')
            p.write_text('ELEVENLABS_API_KEY=two keys\n')
            with self.assertRaises(client.AudioError): client.load_key(p)

    def test_errors_never_echo_credentials(self):
        secret = 'private-test-credential'
        error = HTTPError('https://api.elevenlabs.io/v1/test', 401, secret, {}, io.BytesIO(secret.encode()))
        with patch.object(client, 'urlopen', side_effect=error):
            with self.assertRaises(client.AudioError) as caught: client.speech(secret, 'Test')
            self.assertNotIn(secret, str(caught.exception))
            self.assertEqual(caught.exception.status, 401)
        with patch.object(client, 'urlopen', side_effect=URLError(secret)):
            with self.assertRaises(client.AudioError) as caught: client.speech(secret, 'Test')
            self.assertNotIn(secret, str(caught.exception))

    def test_requests_use_only_elevenlabs_with_header_auth(self):
        with patch.object(client, 'urlopen', side_effect=URLError('offline')) as call:
            with self.assertRaises(client.AudioError): client.speech('test-key', 'Roll call')
            request = call.call_args[0][0]
            self.assertTrue(request.full_url.startswith('https://api.elevenlabs.io/v1/text-to-speech/'))
            self.assertEqual(request.headers['Xi-api-key'], 'test-key')
            self.assertNotIn('test-key', request.full_url)
            self.assertNotIn('test-key', request.data.decode())
            self.assertEqual(json.loads(request.data)['text'], 'Roll call')

    def test_atomic_reports(self):
        with tempfile.TemporaryDirectory() as work:
            p = Path(work) / 'result.json'
            client.write_json(p, {'ok':False, 'http_status':0})
            self.assertEqual(json.loads(p.read_text()), {'ok':False, 'http_status':0})
            self.assertEqual(len(list(Path(work).iterdir())), 1)

if __name__ == '__main__': unittest.main()
