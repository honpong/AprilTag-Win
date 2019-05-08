"""Configure packaging.

"""

from setuptools import setup, find_packages

VERSION = '1.0.0'

setup(name='python-delairstack',
      version=VERSION,
      license='MIT',
      description='High-level Python interface to Delair-Stack API',
      author='Mathieu Dhainaut',
      author_email='mathieu.dhainaut@delair.aero',
      url='http://srvdev.delair.local/delair-stack/python-delairstack',
      install_requires=['urllib3', 'requests-futures'],
      packages=find_packages(exclude=['docs', 'tests*']),
      package_dir={'delairstack': 'delairstack'},
      package_data={
          'delairstack': ['logging.conf',
                          'core/config-connection.json',
                          'core/config-resources.json']
      },
      test_suite='tests',
      extras_require={
          'development': ['pycodestyle'],
          'coverage': ['coverage>=4.4'],
          'documentation': ['sphinx']
      })
