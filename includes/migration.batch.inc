<?php
/**
 * @file
 * Migration batching code for Boston using the Batch interface.
 */

class BostonCollegeBatchPreprocessor extends IslandoraScanBatch {

  /**
   * Sets recursive scasn to false.
   *
   * @see IslandoraScanBatch::__consruct
   */
  public function __construct(IslandoraTuque $connection, array $parameters) {
    parent::__construct($connection, $parameters);
    $this->recursiveScan = FALSE;
  }

  /**
   * Get the pattern to sniff files for.
   *
   * @see IslandoraScanBatch::getPattern()
   */
  protected static function getPattern() {
    return '/\.xml$/';
  }

  /**
   * Get the associated object class.
   *
   * @see IslandoraScanBatch:getObjectClass()
   */
  protected static function getObjectClass() {
    return 'BostonCollegeBatchObject';
  }
}

class BostonCollegeBatchObject extends IslandoraScanBatchObject {
  protected $sourceXML;
  protected $type;
  protected $pdfURI;
  protected static $typeMappings;
  /**
   * Constructs an object for Boston College.
   *
   * @see IslandoraScanBatch::__construct
   */
  public function __construct(IslandoraTuque $connection, $base_name, $object_info, $preprocessor_parameters) {
    parent::__construct($connection, $base_name, $object_info, $preprocessor_parameters);
    if (!static::$typeMappings) {
      static::$typeMappings = array(
        'dc' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/DC_MODS3-4_XSLT1-0.xsl',
        ),
        'etd-ms' => array(
          'content_model' => 'ir:graduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/ETDMS_MODS3-4_XSLT1-0.xsl',
        ),
        'modstext' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => 'MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
        'modsvideo' => array(
          'content_model' => 'bc:VIDEO',
          'collection' => 'ir:citation',
          'transform' => 'MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
      );
    }
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $type = $this->determineType($xml);
    $this->type = $type;
    if ($type) {
      if (isset(static::$typeMappings[$type])) {
        $this->preprocessorParameters['content_models'] = array(static::$typeMappings[$type]['content_model']);
      }
    }
    else {
      drush_log(t('Unable to determine the type for @file.', array('@file' => $this->objectInfo['xml']->uri)));
    }
    $archival_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/stream_ref[file_extension/text() = "pdf"]/file_name');
    if ($archival_xpath) {
      $pdf_name = (string) reset($archival_xpath);
      $this->pdfURI = $this->preprocessorParameters['target'] . '/' . $pdf_name;
    }
    else {
      if ($type != 'modsvideo') {
        drush_log(t('Unable to match a PDF for @file', array('@file' => $this->objectInfo['xml']->uri)));
      }
    }
  }

  /**
   * Wakey, wakey.
   */
  public function __wakeup() {
    parent::__wakeup();
    if (!static::$typeMappings) {
      static::$typeMappings = array(
        'dc' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/DC_MODS3-4_XSLT1-0.xsl',
        ),
        'etd-ms' => array(
          'content_model' => 'ir:graduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/ETDMS_MODS3-4_XSLT1-0.xsl',
        ),
        'modstext' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => 'MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
        'modsvideo' => array(
          'content_model' => 'bc:VIDEO',
          'collection' => 'ir:citation',
          'transform' => 'MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
      );
    }
  }

  /**
   * Process the batch.
   */
  public function batchProcess() {
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $this->sourceXML = $xml;
    $this->getArchivalPDF();

    if ($this->type == 'dc') {
      $this->getEventHistory();
    }

    if ($this->type == 'modsvideo') {
      $this->getArchivalVideo();
    }
    return parent::batchProcess();
  }

  /**
   * Determines the content model based upon the source XML from DigiTools.
   *
   * @param SimpleXMLElement $xml
   *   A SimpleXMLElement containing our loaded source xml.
   *
   * @return bool|string
   *   FALSE if the XPath fails, the content model string otherwise.
   */
  public function determineType($xml) {
    $type_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[name/text() = "descriptive"]/type');
    if ($type_xpath) {
      $type = (string) reset($type_xpath);
      if ($type == 'mods') {
        $entity_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/entity_type/text()');
        if ($entity_xpath) {
          $entity = (string) reset($entity_xpath);
          if ($entity == 'VIDEO_STREAM') {
            $type = 'modsvideo';
          }
          else {
            $type = 'modstext';
          }
        }
      }
      return $type;
    }
    return FALSE;
  }

  /**
   * Go out and create the MODS.
   */
  protected function getMods() {
    if (!isset($this['MODS'])) {
      // Get the data to be transformed.
      $source_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[name/text() = "descriptive"]/value');
      if ($source_xpath) {
        // Take the source XML and transform it using the supplied XSLs.
        $source = (string) reset($source_xpath);
        $mods_xml = static::runXslTransform(array(
          'input' => $source,
          'xsl' => static::$typeMappings[$this->type]['transform'],
        ));

        $mods_doc = new DOMDocument();
        $mods_doc->loadXML($mods_xml);

        $mods_to_add = array();

        // Go get the Handle value from PREMIS if it exists and append.
        $premis_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[type/text() = "preservation_md"]/value');
        if ($premis_xpath) {
          $premis = (string) reset($premis_xpath);
          $premis_xml = simplexml_load_string($premis);
          $premis_xml->registerXPathNamespace('premis', 'http://www.loc.gov/standards/premis');
          $handle_result = $premis_xml->xpath('/premis:object/premis:objectIdentifier[premis:objectIdentifierType/text() = "handle"]/premis:objectIdentifierValue/text()');
          if ($handle_result) {
            $handle = (string) reset($handle_result);
            $mods_to_add[] = array(
              'field' => 'mods:identifier',
              'attribute' => array(
                'name' => 'type',
                'value' => 'hdl',
              ),
              'value' => "http://hdl.handle.net/$handle",
            );
          }
        }
        // Finally, add the digiTool PID as an identifier.
        $digi_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/pid/text()');
        if ($digi_xpath) {
          $digi_pid = (string) reset($digi_xpath);
          $mods_to_add[] = array(
            'field' => 'mods:identifier',
            'attribute' => array(
              'name' => 'type',
              'value' => 'digitool',
            ),
            'value' => $digi_pid,
          );
        }

        // For videos we need to set the location.
        if ($this->type == 'modsvideo') {
          
        }

        foreach ($mods_to_add as $values) {
          $element = $mods_doc->createElement($values['field']);
          if (isset($values['attribute'])) {
            $attribute = $mods_doc->createAttribute($values['attribute']['name']);
            $attribute->value = $values['attribute']['value'];
            $element->appendChild($attribute);
          }
          $text = $mods_doc->createTextNode($values['value']);
          $element->appendChild($text);
          $mods_doc->documentElement->appendChild($element);
        }
        $mods_xml = $mods_doc->saveXML();
        $mods_datastream = $this->constructDatastream('MODS', 'M');
        $mods_datastream->mimetype = 'text/xml';
        $mods_datastream->label = 'MODS Record';
        $mods_datastream->content = $mods_xml;
        $this->ingestDatastream($mods_datastream);
        return $this['MODS']->content;
      }
      else {
        drush_log(t('Unable to retrieve source metadata for @file.', array('@file' => $this->objectInfo['xml']->uri)));
      }
    }
    else {
      return $this['MODS']->content;
    }
  }

  /**
   * Gets the archival PDF to be added.
   */
  protected function getArchivalPDF() {
    if ($this->pdfURI) {
      $pdf_datastream = $this->constructDatastream(BOSTON_COLLEGE_ARCHIVAL_PDF_DATASTREAM_ID, 'M');
      $pdf_datastream->mimetype = 'application/pdf';
      $pdf_datastream->label = 'PDF Datastream';
      $pdf_datastream->setContentFromFile($this->pdfURI);
      $this->ingestDatastream($pdf_datastream);
    }
  }

  /**
   * Creates the event history datastream for graduate objects.
   */
  protected function getEventHistory() {
    $event_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[type/text() = "changehistory_md"]/value');
    if ($event_xpath) {
      $history = (string) reset($event_xpath);
      $history = simplexml_load_string($history);
      $history->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
      $deposit_xpath = $history->xpath('/xb:history/events/event[eventIdentifier/eventIdentifierType/text() = "DEPOSIT ID"]');
      if ($deposit_xpath) {
        $deposit = reset($deposit_xpath);
        $archive_datastream = $this->constructDatastream('ARCHIVE', 'M');
        $archive_datastream->mimetype = 'application/xml';
        $archive_datastream->label = 'Event History';
        $archive_datastream->content = $deposit->saveXML();
        $this->ingestDatastream($archive_datastream);
      }
    }
  }

  protected function getArchivalVideo() {

  }

  /**
   * Add relationships to the object.
   *
   * @see IslandoraScanBatch::addRelationships()
   */
  public function addRelationships() {
    parent::addRelationships();
    // Grab the digiTools creation date to slam into the RELS-EXT.
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $creation_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/creation_date/text()');
    if ($creation_xpath) {
      $date = (string) reset($creation_xpath);
      $formatted_date = gmdate("Y-m-d\TH:i:s\Z", strtotime($date));
      $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'digiToolCreationDate', $formatted_date, RELS_TYPE_DATETIME);
    }
    else {
      drush_log(t('Unable to add creation date to the RELS-EXT for @file', array('@file' => $this->objectInfo['xml']->uri)));
    }
  }

  /**
   * No resources needed.
   *
   * @return array
   *   The empty array of no resources.
   */
  public function getResources() {
    return array();
  }

}
