<?php

/**
 * Creates meta tags to be placed in the head of the HTML.
 *
 * @param AbstractObject $object
 *   A AbstractObject.
 *
 * @return array
 *   Associative array where the key is the name of the tag and the value is
 *   the content.
 */
function boston_college_islandora_google_scholar_create_meta_tags($object) {
  if (!isset($object['MODS']) || !islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $object['MODS'])) {
    return FALSE;
  }
  else {
   
    $mods = $object['MODS']->content;
    $mods_xml = new SimpleXMLElement($mods);
    $mods_xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
    $mods_xml->registerXPathNamespace('etdms', 'http://www.ndltd.org/standards/metadata/etdms/1.0/');
       
    // Return empty tag set if author title or date are missing
    $authors = array();
    $citation_author = $mods_xml->xpath('/mods:mods/mods:name[@usage="primary"][mods:role/mods:roleTerm[@type="code"]="aut"]/mods:displayForm');
    
    foreach ($citation_author as $name) {
        $author = (string) reset($name);
        $tags['citation_author'][] = $author;
    }
    
    if (count($tags['citation_author']) == 0) {
      return FALSE;
    }
    
    //title
    $citation_nonSort = $mods_xml->xpath('/mods:mods/mods:titleInfo[@usage="primary"]/mods:nonSort');
    $citation_title = $mods_xml->xpath('/mods:mods/mods:titleInfo[@usage="primary"]/mods:title');
    $citation_subTitle = $mods_xml->xpath('/mods:mods/mods:titleInfo[@usage="primary"]/mods:subTitle');           
    
    $nonSort = (string) reset($citation_nonSort);
    $title = (string) reset($citation_title);
    $subtitle = (string) reset($citation_subTitle);        
    
    $fulltitle = $title;
    
    if ($nonSort) {
       $fulltitle = $nonSort . $fulltitle;
    }
    
    if ($subtitle) {
       $fulltitle = $fulltitle . ': ' . $subtitle;
    }
    
    if (!empty($fulltitle)){
      $tags['citation_title'] = $fulltitle;
    } else {
        return FALSE;
    }
    
    //date
    $citation_date = $mods_xml->xpath('/mods:mods/mods:originInfo/mods:dateIssued[@keyDate="yes"]');
    $date = (string) reset($citation_date);
    
    if (empty($date)) {
      $citation_date = $mods_xml->xpath('/mods:mods/mods:originInfo/mods:dateCreated[@keyDate="yes"]');
      $date = (string) reset($citation_date);
    }
    
    if (!empty($date)) {
      $tags['citation_date'] = $date;
    } else {
        return FALSE;
    }
    
    //citation_pdf_url
    $pid = $object->id;
    
     if (!empty($pid)) {
      $tags['citation_pdf_url'] = "http://dlib.bc.edu/islandora/object/" . $pid . "/datastream/PDF/view";
      
      //citation_abstract_html_url
      $tags['citation_abstract_html_url'] = "http://dlib.bc.edu/islandora/object/" . $pid;
      
    }   
    
    //citation_publisher
    $citation_publisher = $mods_xml->xpath('/mods:mods/mods:originInfo/mods:publisher');
    $publisher = (string) reset($citation_publisher);

    if (!empty($publisher)) {
      $tags['citation_publisher'] = $publisher;
    }  
    
    //citation_issn
    $citation_issn = $mods_xml->xpath('/mods:mods/mods:relatedItem[@type="host"]/mods:identifier[@type="issn"]');
    $issn = (string) reset($citation_issn);

    if (!empty($issn)) {
      $tags['citation_issn'] = $issn;
      
      //citation_journal_title only if ISSN
      $citation_journal_title = $mods_xml->xpath('/mods:mods/mods:relatedItem[@type="host"]/mods:titleInfo[@usage="primary"]/mods:title');
      $journal_title = (string) reset($citation_journal_title);

      if (!empty($journal_title)) {
        $tags['citation_journal_title'] = $journal_title; 
      }
    } 
    
    //citation_volume
    $citation_volume = $mods_xml->xpath('/mods:mods/mods:relatedItem[@type="host"]/mods:part/mods:detail[@type="volume"]/mods:number');
    $volume = (string) reset($citation_volume);

    if (!empty($volume)) {
      $tags['citation_volume'] = $volume;
    }     
    
    //citation_issue
    $citation_issue = $mods_xml->xpath('/mods:mods/mods:relatedItem[@type="host"]/mods:part/mods:detail[@type="issue"]/mods:number');
    $issue = (string) reset($citation_issue);

    if (!empty($issue)) {
      $tags['citation_issue'] = $issue;
    } 
       
    //citation_firstpage
    $citation_firstpage = $mods_xml->xpath('/mods:mods/mods:relatedItem[@type="host"]/mods:part/mods:extent[@unit="pages"]/mods:start');
    $firstpage = (string) reset($citation_firstpage);

    if (!empty($firstpage)) {
      $tags['citation_firstpage'] = $firstpage;
    } 
    
    //citation_lastpagee
    $citation_lastpage = $mods_xml->xpath('/mods:mods/mods:relatedItem[@type="host"]/mods:part/mods:extent[@unit="pages"]/mods:end');
    $lastpage = (string) reset($citation_lastpage);

    if (!empty($lastpage)) {
      $tags['citation_lastpage'] = $lastpage;
    } 
    
    //citation_language
    $citation_language = $mods_xml->xpath('/mods:mods/mods:language/mods:languageTerm[@type="text"][@authority="iso639-2b"]');
    $language = (string) reset($citation_language);

    if (!empty($language)) {
      $tags['citation_language'] = $language;
    }

    //citation_dissertation_institution
    $citation_dissertation_institution = $mods_xml->xpath('/mods:mods/mods:extension/etdms:degree/etdms:grantor');
    $dissertation_institution = (string) reset($citation_dissertation_institution);

    if (!empty($dissertation_institution)) {
      $tags['citation_dissertation_institution'] = $dissertation_institution;
    }
    
    //citation_dissertation_name
    $citation_dissertation_name = $mods_xml->xpath('/mods:mods/mods:extension/etdms:degree/etdms:level');
    $dissertation_name = (string) reset($citation_dissertation_name);

    if (!empty($dissertation_name)) {
      $tags['citation_dissertation_name'] = $dissertation_name;
    }
    
  }    
  return $tags;
}
